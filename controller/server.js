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
const RECORDER_PATH = "/app/recorder-bin/recorder";
const RECORDING_ROOT = "/app/output";

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
    const { channel, channelKey, uid, participants, useProxy} = req.body;
    console.log('useProxy', useProxy);
    const videoWidth = 640;

    console.log("Incoming start request:", req.body);

    if (!channel || !channelKey || !uid) {
        return res.status(400).json({ error: "Missing parameters" });
    }

    if (sessions[channel]) {
        return res.status(400).json({
            error: "This channel is already being recorded",
            sid: sessions[channel].sid
        });
    }


    const wmConfigs = participants ? participants.map((p, i) => {
        const userLeftX = i * videoWidth;

        return {
            config: {
                litera: {
                    wm_litera: p.name,
                    font_size: 10,
                    offset_x: userLeftX + 20,
                    offset_y: 200, 
                    wm_width: 150,
                    wm_height: 30
                }
            }
        };
    }) : [];
    
    const sid = generateSID();

    const args = [
        "--appId", process.env.AGORA_APP_ID,
        "--channel", channel,
        "--uid", uid,
        "--channelKey", channelKey,
        "--recordFileRootDir", RECORDING_ROOT,
        "--isMixingEnabled", "1",
        "--autoSubscribe", "1",
        "--idle", "60",
	"--wmNum", wmConfigs.length.toString(),
        "--wmConfigs", JSON.stringify(wmConfigs)
    ];

    if (process.env.INTERNAL_PROXY && process.env.INTERNAL_PROXY.trim() !== "") {
 
        // Gunakan Internal Proxy
        args.push("--proxyServer", process.env.INTERNAL_PROXY.trim());
        console.log("PRIORITY: Using Internal Proxy:", process.env.INTERNAL_PROXY);

    } 
    // 2. CEK CLOUD PROXY HANYA JIKA INTERNET PROXY KOSONG
    else if (useProxy === true || useProxy === "1") {
    
        // Gunakan Agora Cloud Proxy
        args.push("--enableCloudProxy", "1");
        console.log("SECONDARY: Using Agora Cloud Proxy");

    } else {
        console.log("No Proxy configured.");
    }

    console.log("Recorder args:", args);

    const recorderProcess = spawn(RECORDER_PATH, args);

    sessions[channel] = { sid, process: recorderProcess };

    recorderProcess.stdout.on("data", d =>
        console.log(`[${channel}] STDOUT: ${d}`)
    );

    recorderProcess.stderr.on("data", d =>
        console.log(`[${channel}] STDERR: ${d}`)
    );

    recorderProcess.on("exit", code => {
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

    const channel = Object.keys(sessions)
        .find(c => sessions[c].sid === sid);

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

    return fs.readdirSync(dir, { withFileTypes: true })
        .filter(d => d.isDirectory())
        .map(d => ({
            name: d.name,
            path: path.join(base, d.name),
            children: scanDir(
                path.join(dir, d.name),
                path.join(base, d.name)
            )
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

    const files = fs.readdirSync(dir)
        .filter(f => f.endsWith(".mp4"))
        .map(f => {
            const full = path.join(dir, f);
            const stat = fs.statSync(full);
            return {
                name: f,
                path: path.join(safePath, f),
                sizeMB: (stat.size / 1024 / 1024).toFixed(2)
            };
        });

    res.json(files);
});

/* =========================
   START SERVER
========================= */
app.listen(PORT, () => {
    console.log(`Agora Recorder Controller running on port ${PORT}`);
});

