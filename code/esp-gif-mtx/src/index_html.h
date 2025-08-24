#ifndef INDEX_HTML_H
#define INDEX_HTML_H

// THIS FILE HAS BEEN GENERATED, DO NOT EDIT!
// This files has been generated from resources/index.html with scripts/gen_index.sh
// Important: `)"` has to be written with a space like `) "` or escaping breaks, only the end of the html code should be written like `)`;
const char* htmlCode = R"(
<!DOCTYPE html>
<!-- Important: use semicolon in onclick function calls to avoid having `)` and `"` next to each other -->
<html>
<head>
    <title>LED Matrix GIF Upload</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 40px; }
        .container { max-width: 600px; margin: 0 auto; }
        .upload-form { border: 2px dashed #ccc; padding: 20px; text-align: center; margin-bottom: 20px; }
        .upload-form.dragover { border-color: #007cba; background-color: #f0f8ff; }
        button { background: #007cba; color: white; padding: 10px 20px; border: none; cursor: pointer; margin: 5px; }
        button:hover { background: #005a85; }
        button:disabled { background: #ccc; cursor: not-allowed; }
        .status { margin: 20px 0; padding: 15px; background: #f0f0f0; border-radius: 5px; }
        .info { font-size: 12px; color: #666; margin: 10px 0; }
        input[type="number"] { padding: 5px; margin: 5px; width: 60px; }
        .controls { margin: 10px 0; }
        #statusText { color: #007cba; font-weight: bold; display: block; background-color: #fff; padding:20px; }
    </style>
</head>
<body>
    <div class="container">
        <h1>LED Matrix GIF Player</h1>
        <div class="upload-form" id="dropZone">
            <p>Drop a GIF file here or click to select:</p>
            <input type="file" id="gifFile" accept=".gif" required style="display: none;">
            <br><br>
            <button id="uploadBtn" onclick="document.getElementById('gifFile').click(); ">Select GIF</button>
        </div>
        <div class="status">
            <div class="controls">
                <button onclick="apiCall('/start'); ">Start Playback</button>
                <button onclick="apiCall('/stop'); ">Stop Playback</button>
                <button onclick="apiCall('/info'); ">Memory Info</button>
            </div>
            <div class="controls">
                Global Frame Delay (ms): <input type="number" id="globalDelay" min="0" value="0">
                <button onclick="setGlobalDelay(); ">Set</button>
            </div>
            <div class="controls">
                Last Frame Delay (ms): <input type="number" id="lastDelay" min="0" value="0">
                <button onclick="setLastDelay(); ">Set</button>
            </div>
            <div class="controls">
                Frame: <input type="number" id="frameIndex" min="0" value="0">
                Delay (ms): <input type="number" id="frameDelay" min="0" value="0">
                <button onclick="setFrameDelay(); ">Set Frame Delay</button>
            </div>

            <div><hr/></div>

            <div class="controls">
                Brightness: <input type="number" id="brightness" min="0" max="255" value="10">
                <button onclick="setBrightness(); ">Set Brightness</button>
            </div>
            <!-- <div class="controls">
                Rotation: 
                <select id="rotation">
                    <option value="0">0°</option>
                    <option value="1">90°</option>
                    <option value="2">180°</option>
                    <option value="3">270°</option>
                </select>
                <button onclick="setRotation(); ">Set Rotation</button>
            </div>
            
            <div><hr/></div>

            <div class="controls">
                Text: <input type="text" id="text" value="Hello World">
                <button onclick="setText(); ">Display Text</button>
                <button onclick="clearText(); ">Clear Text</button>
            </div> -->

            <div id="statusText">Ready</div>
        </div>
    </div>
    
<script>

let hostOverride = ''; // set to for example 'http://10.0.0.126' to point to the esp32 from another device

function setStatus(msg) { document.getElementById('statusText').innerText = msg; }

// Drag and drop functionality
const dropZone = document.getElementById('dropZone');
const fileInput = document.getElementById('gifFile');

dropZone.addEventListener('dragover', (e) => {
    e.preventDefault();
    dropZone.classList.add('dragover');
});

dropZone.addEventListener('dragleave', () => {
    dropZone.classList.remove('dragover');
});

dropZone.addEventListener('drop', (e) => {
    e.preventDefault();
    dropZone.classList.remove('dragover');
    
    if (e.dataTransfer.files.length) {
        fileInput.files = e.dataTransfer.files;
        uploadGif();
    }
});


async function uploadGif() {
    const fileInput = document.getElementById('gifFile');
    const uploadBtn = document.getElementById('uploadBtn');
    
    if (!fileInput.files[0]) {
        setStatus('Please select a GIF file');
        return;
    }
    
    const file = fileInput.files[0];
    if (!file.name.toLowerCase().endsWith('.gif')) {
        setStatus('Please select a GIF file');
        return;
    }
    
    const formData = new FormData();
    formData.append('gif', file);
    
    uploadBtn.disabled = true;
    setStatus('Uploading...');
    
    try {
        const response = await fetch(hostOverride + '/upload', {
            method: 'POST',
            body: formData
        });
        
        const result = await response.text();
        setStatus(result);
        
        if (response.ok) {
            fileInput.value = '';
        }
    } catch (e) {
        setStatus('Upload failed: ' + e.message);
    } finally {
        uploadBtn.disabled = false;
    }
}

// Add this event listener to automatically upload when a file is selected
document.getElementById('gifFile').addEventListener('change', function() {
    if (this.files.length > 0) {
        uploadGif();
    }
});


async function apiCall(endpoint) {
    try {
        const response = await fetch(hostOverride + endpoint);
        const result = await response.text();
        setStatus(result);
    } catch (e) {
        setStatus('Error: ' + e.message);
    }
}

function setGlobalDelay() {
    const delay = document.getElementById('globalDelay').value;
    apiCall('/set-global-delay?delay=' + delay);
}

function setLastDelay() {
    const delay = document.getElementById('lastDelay').value;
    apiCall('/set-last-delay?delay=' + delay);
}

function setFrameDelay() {
    const index = document.getElementById('frameIndex').value;
    const delay = document.getElementById('frameDelay').value;
    apiCall('/set-frame-delay?index=' + index + '&delay=' + delay);
}
function setBrightness() {
    const brightness = document.getElementById('brightness').value;
    apiCall('/set-brightness?brightness=' + brightness);
}

function setRotation() {
    const rotation = document.getElementById('rotation').value;
    apiCall('/set-rotation?rotation=' + rotation);
}

function setText() {
    const text = document.getElementById('text').value;
    apiCall('/set-text?text=' + encodeURIComponent(text));
}

function clearText() {
    apiCall('/set-text'); // Call with no text parameter to clear
}
</script>
</body>
</html>
)";

#endif
