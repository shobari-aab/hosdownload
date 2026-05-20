const express = require("express");
const fs = require("fs");
const { spawn } = require("child_process");
const path = require("path");
require("dotenv").config();

const app = express();
app.use(express.json());

/* =========================
   CONSTANTS
========================= */
const PORT = 7070;
const RECORDER_PATH = process.env.RECORDER_PATH || "/app/recorder-bin/recorder";
const RECORDING_ROOT = process.env.RECORDING_ROOT || "/app/output";

/* =========================
   STATIC FILES
========================= */
// Serve mp4 files
app.use("/recordings", express.static(RECORDING_ROOT));

/* =========================
   MULTI-SESSION STORAGE
========================= */
// sessions[channel] = { sid, process }
let sessions = {};

function generateSID() {
  return Math.random().toString(36).substring(2, 12);
}

/* =========================
   START RECORDING
========================= */
app.post("/start", (req, res) => {
  // 1. Ambil data dari body (pastikan key-nya sesuai dengan yang dikirim frontend)
  const {
    channel,
    channelKey,
    uid,
    hostUid, // UID Agent/Host (Jendela kecil)
    guestUid, // UID Customer/Guest (Latar penuh)
    callID: rawCallID, // Call ID of the session
    useProxy,
  } = req.body;

  console.log("Incoming start request:", req.body);

  if (!channel || !channelKey || !uid) {
    return res.status(400).json({ error: "Missing parameters" });
  }

  if (sessions[channel]) {
    return res.status(400).json({
      error: "This channel is already being recorded",
      sid: sessions[channel].sid,
    });
  }

  const callID = rawCallID || `call_${Date.now()}`; // Default callID jika tidak diberikan

  const sid = generateSID();
  // 2. Susun Argumen untuk C++ Binary
  // Perhatikan kita menambahkan --hostUid dan --guestUid
  const args = [
    "--appId",
    process.env.AGORA_APP_ID,
    "--channel",
    channel,
    "--uid",
    uid,
    "--channelKey",
    channelKey,
    "--recordFileRootDir",
    RECORDING_ROOT,
    "--isMixingEnabled",
    "1",
    "--autoSubscribe",
    "1",
    "--idle",
    "60",
    "--hostUid",
    hostUid ? hostUid.toString() : "0",
    "--guestUid",
    guestUid ? guestUid.toString() : "0",
    "--callID",
    callID,
  ];

  // 3. Logika Proxy
  if (process.env.INTERNAL_PROXY && process.env.INTERNAL_PROXY.trim() !== "") {
    args.push("--proxyServer", process.env.INTERNAL_PROXY.trim());
    console.log("PRIORITY: Using Internal Proxy:", process.env.INTERNAL_PROXY);
  } else if (useProxy === true || useProxy === "1") {
    args.push("--enableCloudProxy"); // Di C++ kita pakai no_argu untuk enableCloudProxy
    console.log("SECONDARY: Using Agora Cloud Proxy");
  } else {
    console.log("No Proxy configured.");
  }

  console.log("Final Recorder args:", args);

  // 4. Jalankan Proses Recorder
  const recorderProcess = spawn(RECORDER_PATH, args);

  sessions[channel] = { sid, process: recorderProcess };

  recorderProcess.stdout.on("data", (d) =>
    console.log(`[${channel}] STDOUT: ${d}`),
  );

  recorderProcess.stderr.on("data", (d) =>
    console.log(`[${channel}] STDERR: ${d}`),
  );

  recorderProcess.on("exit", (code) => {
    console.log(`Recorder exited for ${channel} (code ${code})`);
    delete sessions[channel];
  });

  return res.json({ sid });
});

/* =========================
   STOP RECORDING
========================= */
app.post("/stop", (req, res) => {
  const { sid } = req.body;
  if (!sid) return res.status(400).json({ error: "SID required" });

  const channel = Object.keys(sessions).find((c) => sessions[c].sid === sid);

  if (!channel) {
    return res.status(404).json({ error: "No active session" });
  }

  console.log(`Stopping recording: channel=${channel}, SID=${sid}`);

  try {
    sessions[channel].process.kill("SIGINT");
  } catch (e) {
    console.error("Kill failed:", e);
  }

  delete sessions[channel];
  res.json({ message: "Stopped", sid });
});

/* =========================
   RECORDING UI
========================= */
app.get("/recording", (req, res) => {
  res.sendFile(path.join(__dirname, "views", "recordings.html"));
});

/* =========================
   FOLDER TREE API (LEFT)
========================= */
function scanDir(dir, base = "") {
  if (!fs.existsSync(dir)) return [];

  return fs
    .readdirSync(dir, { withFileTypes: true })
    .filter((d) => d.isDirectory())
    .map((d) => ({
      name: d.name,
      path: path.join(base, d.name),
      children: scanDir(path.join(dir, d.name), path.join(base, d.name)),
    }));
}

app.get("/api/tree", (req, res) => {
  res.json(scanDir(RECORDING_ROOT));
});

/* =========================
   FILE LIST API (RIGHT)
========================= */
app.get("/api/files", (req, res) => {
  const relPath = req.query.path || "";
  const safePath = path.normalize(relPath).replace(/^(\.\.(\/|\\|$))+/, "");
  const dir = path.join(RECORDING_ROOT, safePath);

  if (!fs.existsSync(dir)) return res.json([]);

  const files = fs
    .readdirSync(dir)
    .filter((f) => f.endsWith(".mp4"))
    .map((f) => {
      const full = path.join(dir, f);
      const stat = fs.statSync(full);
      return {
        name: f,
        path: path.join(safePath, f),
        sizeMB: (stat.size / 1024 / 1024).toFixed(2),
      };
    });

  res.json(files);
});

/* =========================
   RETENTION SCHEDULER
========================= */
const RETENTION_DAYS = parseInt(process.env.RETENTION_DAYS) || 60;
const RETENTION_MS = RETENTION_DAYS * 24 * 60 * 60 * 1000;
const SCHEDULER_INTERVAL_MS =
  (parseInt(process.env.RETENTION_CHECK_HOURS) || 24) * 60 * 60 * 1000;

function deleteEmptyDirs(dir) {
  if (!fs.existsSync(dir)) return;
  let entries = fs.readdirSync(dir);
  for (const entry of entries) {
    const full = path.join(dir, entry);
    if (fs.statSync(full).isDirectory()) {
      deleteEmptyDirs(full);
    }
  }
  entries = fs.readdirSync(dir);
  if (entries.length === 0 && dir !== RECORDING_ROOT) {
    fs.rmdirSync(dir);
    console.log(`[Retention] Hapus folder kosong: ${dir}`);
  }
}

function runRetention() {
  console.log(`[Retention] Mulai pengecekan. Retensi: ${RETENTION_DAYS} hari`);
  if (!fs.existsSync(RECORDING_ROOT)) return;

  const now = Date.now();
  let deleted = 0;

  function scanAndDelete(dir) {
    if (!fs.existsSync(dir)) return;
    for (const entry of fs.readdirSync(dir, { withFileTypes: true })) {
      const full = path.join(dir, entry.name);
      if (entry.isDirectory()) {
        scanAndDelete(full);
      } else if (entry.name.endsWith(".mp4")) {
        const stat = fs.statSync(full);
        if (now - stat.mtimeMs > RETENTION_MS) {
          fs.unlinkSync(full);
          console.log(`[Retention] Hapus: ${full}`);
          deleted++;
        }
      }
    }
  }

  scanAndDelete(RECORDING_ROOT);
  deleteEmptyDirs(RECORDING_ROOT);
  console.log(`[Retention] Selesai. Total file dihapus: ${deleted}`);
}

// Jalankan sekali saat startup, lalu setiap 24 jam
runRetention();
setInterval(runRetention, SCHEDULER_INTERVAL_MS);

/* =========================
   START SERVER
========================= */
app.listen(PORT, () => {
  console.log(`Agora Recorder Controller running on port ${PORT}`);
});
