#ifndef INDEX_HTML_H
#define INDEX_HTML_H


const char* getIndexHTML() {
    static const char html[] = R"rawliteral(
        <!DOCTYPE html><html lang="en"><meta charset="UTF-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>Shakal Cam</title><link rel="icon" type="image/png" id="favicon" href="#"><style>:root{--font-size:24px;--background-color:#000;--text-color:#fff}body,html{width:100%;height:100%;margin:0;padding:0;background-color:var(--background-color);font-family:monospace}.container{width:100%;height:100%;display:flex;justify-content:center;align-items:center;position:relative}.controls{position:absolute;bottom:20px;left:20px;right:20px;display:flex;justify-content:space-between;align-items:center;gap:1rem}.controls__group{display:flex;align-items:center;gap:1rem}.time-elapsed{color:var(--text-color);font-size:var(--font-size);font-weight:700;margin-left:10px}.button{background-color:var(--text-color);color:var(--background-color);border:none;padding:10px 20px;font-size:var(--font-size);font-weight:700;cursor:pointer;font-family:monospace;box-shadow:5px 5px 0 0 var(--background-color)/50%}.button:active{box-shadow:0 0 0 0 var(--background-color)/50%;transform:translate(5px,5px)}.image{width:100%;height:100%;aspect-ratio:1.5;object-fit:cover}@media (max-width:768px){.controls{top:10px;bottom:10px;flex-direction:column;--font-size:18px}}</style><div class="container"><img src="#" width="640" height="480" class="image js-image" onload=""><div class="controls"><div class="controls__group"><span class="js-time-elapsed time-elapsed">00:00</span> <span class="js-fps time-elapsed">0 FPS</span></div><div class="controls__group"><button class="button js-flashlight-button">Flash off</button> <button class="button js-control-button">Stop</button> <button class="button js-capture-button">Capture</button> <a href="/gallery" class="button">Gallery</a></div></div></div><script>const INTERVAL_MS=1e3;let currentHost=window.location.host,isPaused=!1,isFlashOn=!1,dataUrl=null;const controlButton=document.querySelector(".js-control-button"),captureButton=document.querySelector(".js-capture-button"),flashlightButton=document.querySelector(".js-flashlight-button"),image=document.querySelector(".js-image"),timeElapsedElement=document.querySelector(".js-time-elapsed"),fpsElement=document.querySelector(".js-fps");function updateImage(){const t=new Date,e=`http://${currentHost}/capture`;image.src=`${e}?t=${t.getTime()}`,image.onload=()=>{image.style.opacity=1;const e=(new Date).getTime()-t;timeElapsedElement.innerText=`${e}ms`;const a=1e3/e;fpsElement.innerText=`${a.toFixed(2)} FPS`,captureImage(image)}}function flash(){return new Promise(((t,e)=>{fetch(`http://${currentHost}/flash`).then((()=>{setTimeout((()=>{fetch(`http://${currentHost}/flash`)}),300)})),setTimeout((()=>{t()}),200)}))}function captureImage(t){const e=document.createElement("canvas");e.width=t.naturalWidth,e.height=t.naturalHeight;e.getContext("2d").drawImage(t,0,0),dataUrl=e.toDataURL("image/png"),localStorage.setItem("lastimage",dataUrl);const a=document.createElement("canvas");a.width=32,a.height=32;a.getContext("2d").drawImage(t,0,0,32,32);const n=a.toDataURL("image/png");document.getElementById("favicon").href=n}localStorage.getItem("lastimage")&&(image.src=localStorage.getItem("lastimage"),image.style.opacity=.5),controlButton.addEventListener("click",(()=>{isPaused=!isPaused,controlButton.innerText=isPaused?"Resume":"Pause"})),captureButton.addEventListener("click",(async()=>{if(!dataUrl)return;isPaused=!0,isFlashOn&&await flash(),updateImage();const t=document.createElement("a");t.href=dataUrl,t.download=`capture_${(new Date).toISOString()}.png`,document.body.appendChild(t),t.click(),document.body.removeChild(t),isPaused=!1})),flashlightButton.addEventListener("click",(()=>{isFlashOn?(isFlashOn=!1,flashlightButton.innerText="Flash off"):(isFlashOn=!0,flashlightButton.innerText="Flash on")})),setInterval((()=>{isPaused||updateImage()}),1e3)</script>
    )rawliteral";

    return html;
}

const char* getGalleryHTML() {
    static const char html[] = R"rawliteral(
        <!DOCTYPE html>
        <html lang="en">
        <head>
            <meta charset="UTF-8">
            <meta name="viewport" content="width=device-width, initial-scale=1.0">
            <title>Shakal Cam Gallery</title>
            <style>
                :root {
                    --background-color: #000;
                    --text-color: #fff;
                    --accent-color: #444;
                    --danger-color: #ff3b30;
                }
                body, html {
                    margin: 0;
                    padding: 0;
                    background-color: var(--background-color);
                    color: var(--text-color);
                    font-family: monospace;
                }
                .header {
                    padding: 20px;
                    display: flex;
                    justify-content: space-between;
                    align-items: center;
                }
                .header h1 {
                    margin: 0;
                }
                .header-buttons {
                    display: flex;
                    gap: 10px;
                }
                .button {
                    background-color: var(--text-color);
                    color: var(--background-color);
                    border: none;
                    padding: 10px 20px;
                    font-size: 18px;
                    font-weight: 700;
                    cursor: pointer;
                    font-family: monospace;
                    text-decoration: none;
                    display: inline-block;
                    box-shadow: 5px 5px 0 0 rgba(0,0,0,0.2);
                }
                .button:hover {
                    box-shadow: 4px 4px 0 0 rgba(0,0,0,0.3);
                }
                .button:active {
                    box-shadow: 0 0 0 0 rgba(0,0,0,0.3);
                    transform: translate(5px,5px);
                }
                .button-danger {
                    background-color: var(--danger-color);
                }
                .storage-info {
                    background-color: var(--accent-color);
                    padding: 10px;
                    margin-bottom: 20px;
                    text-align: center;
                }
                .gallery-container {
                    display: grid;
                    grid-template-columns: repeat(auto-fill, minmax(200px, 1fr));
                    gap: 15px;
                    padding: 20px;
                }
                .gallery-item {
                    position: relative;
                    overflow: hidden;
                    background-color: var(--accent-color);
                    border-radius: 4px;
                    transition: transform 0.2s;
                }
                .gallery-item:hover {
                    transform: scale(1.02);
                }
                .gallery-item a {
                    display: block;
                    text-decoration: none;
                }
                .gallery-item img {
                    width: 100%;
                    aspect-ratio: 1;
                    object-fit: cover;
                    display: block;
                }
                .gallery-item .info {
                    padding: 10px;
                    color: var(--text-color);
                    font-size: 12px;
                    display: flex;
                    justify-content: space-between;
                }
                .modal {
                    display: none;
                    position: fixed;
                    top: 0;
                    left: 0;
                    width: 100%;
                    height: 100%;
                    background-color: rgba(0, 0, 0, 0.7);
                    z-index: 100;
                    align-items: center;
                    justify-content: center;
                }
                .modal-content {
                    background-color: var(--background-color);
                    border: 2px solid var(--text-color);
                    padding: 20px;
                    max-width: 400px;
                    text-align: center;
                }
                .modal-buttons {
                    margin-top: 20px;
                    display: flex;
                    justify-content: center;
                    gap: 10px;
                }
                @media (max-width: 600px) {
                    .gallery-container {
                        grid-template-columns: repeat(auto-fill, minmax(140px, 1fr));
                    }
                    .header {
                        flex-direction: column;
                        gap: 10px;
                    }
                    .header-buttons {
                        width: 100%;
                        flex-direction: column;
                    }
                    .button {
                        width: 100%;
                        text-align: center;
                    }
                }
            </style>
        </head>
        <body>
            <div class="header">
                <h1>Shakal Gallery</h1>
                <div class="header-buttons">
                    <button id="deleteAllBtn" class="button button-danger">Delete All Photos</button>
                    <a href="/" class="button">Back to Camera</a>
                </div>
            </div>
            <div id="storage-info" class="storage-info">
                <!-- Storage info will be inserted here by the ESP32 -->
            </div>
            <div class="gallery-container" id="gallery">
                <!-- Thumbnails will be inserted here by JavaScript -->
            </div>
            
            <!-- Confirmation Modal -->
            <div id="confirmModal" class="modal">
                <div class="modal-content">
                    <h2>Delete All Photos</h2>
                    <p>Are you sure you want to delete all photos? This action cannot be undone.</p>
                    <div class="modal-buttons">
                        <button id="cancelDeleteBtn" class="button">Cancel</button>
                        <button id="confirmDeleteBtn" class="button button-danger">Delete All</button>
                    </div>
                </div>
            </div>

            <script>
                // Get all image files from the server
                async function loadGallery() {
                    const galleryDiv = document.getElementById('gallery');
                    
                    try {
                        const response = await fetch('/photo-list');
                        const data = await response.json();
                        
                        if (data && data.files && data.files.length > 0) {
                            // Sort files by name (newest first if using timestamp naming)
                            data.files.sort().reverse();
                            
                            galleryDiv.innerHTML = data.files.map(file => {
                                const fileSize = file.size ? `${Math.round(file.size / 1024)} KB` : '';
                                return `
                                    <div class="gallery-item">
                                        <a href="/photo/${file.name}" target="_blank">
                                            <img src="/photo/${file.name}" loading="lazy" alt="${file.name}">
                                            <div class="info">
                                                <span>${file.name.split('/').pop()}</span>
                                                <span>${fileSize}</span>
                                            </div>
                                        </a>
                                    </div>
                                `;
                            }).join('');
                            
                            // Update storage info if available
                            if (data.storage) {
                                document.getElementById('storage-info').innerHTML = data.storage;
                            }
                        } else {
                            galleryDiv.innerHTML = '<p style="grid-column: 1/-1; text-align: center;">No photos found</p>';
                        }
                    } catch (error) {
                        console.error('Error loading gallery:', error);
                        galleryDiv.innerHTML = '<p style="grid-column: 1/-1; text-align: center;">Error loading gallery</p>';
                    }
                }
                
                // Delete all photos function
                async function deleteAllPhotos() {
                    try {
                        const response = await fetch('/delete-all-photos', {
                            method: 'POST'
                        });
                        
                        const data = await response.json();
                        if (data.success) {
                            // Reload the gallery to show the updated (empty) state
                            loadGallery();
                            alert(`Successfully deleted ${data.deletedFiles} photos`);
                        } else {
                            alert('Failed to delete photos');
                        }
                    } catch (error) {
                        console.error('Error deleting photos:', error);
                        alert('Error deleting photos. Please try again.');
                    }
                }
                
                // Modal control
                document.addEventListener('DOMContentLoaded', function() {
                    const modal = document.getElementById('confirmModal');
                    const deleteAllBtn = document.getElementById('deleteAllBtn');
                    const cancelDeleteBtn = document.getElementById('cancelDeleteBtn');
                    const confirmDeleteBtn = document.getElementById('confirmDeleteBtn');
                    
                    // Show modal when delete button is clicked
                    deleteAllBtn.addEventListener('click', function() {
                        modal.style.display = 'flex';
                    });
                    
                    // Hide modal when cancel is clicked
                    cancelDeleteBtn.addEventListener('click', function() {
                        modal.style.display = 'none';
                    });
                    
                    // Delete all photos when confirmed
                    confirmDeleteBtn.addEventListener('click', function() {
                        modal.style.display = 'none';
                        deleteAllPhotos();
                    });
                    
                    // Close modal if clicked outside
                    window.addEventListener('click', function(event) {
                        if (event.target === modal) {
                            modal.style.display = 'none';
                        }
                    });
                    
                    // Load gallery on page load
                    loadGallery();
                });
            </script>
        </body>
        </html>
    )rawliteral";

    return html;
}

const char* getSettingsHTML() {
    static const char html[] = R"rawliteral(
        <!DOCTYPE html>
        <html lang="en">
        <head>
            <meta charset="UTF-8">
            <meta name="viewport" content="width=device-width, initial-scale=1.0">
            <title>Shakal Cam Settings</title>
            <style>
                :root {
                    --background-color: #000;
                    --text-color: #fff;
                    --accent-color: #444;
                    --success-color: #28a745;
                }
                body, html {
                    margin: 0;
                    padding: 0;
                    background-color: var(--background-color);
                    color: var(--text-color);
                    font-family: monospace;
                }
                .header {
                    padding: 20px;
                    display: flex;
                    justify-content: space-between;
                    align-items: center;
                }
                .header h1 {
                    margin: 0;
                }
                .header-buttons {
                    display: flex;
                    gap: 10px;
                }
                .button {
                    background-color: var(--text-color);
                    color: var(--background-color);
                    border: none;
                    padding: 10px 20px;
                    font-size: 18px;
                    font-weight: 700;
                    cursor: pointer;
                    font-family: monospace;
                    text-decoration: none;
                    display: inline-block;
                    box-shadow: 5px 5px 0 0 rgba(0,0,0,0.2);
                }
                .button:hover {
                    box-shadow: 4px 4px 0 0 rgba(0,0,0,0.3);
                }
                .button:active {
                    box-shadow: 0 0 0 0 rgba(0,0,0,0.3);
                    transform: translate(5px,5px);
                }
                .button-success {
                    background-color: var(--success-color);
                    color: #fff;
                }
                .settings-container {
                    padding: 20px;
                    max-width: 800px;
                    margin: 0 auto;
                }
                .settings-section {
                    margin-bottom: 30px;
                    background-color: var(--accent-color);
                    padding: 20px;
                    border-radius: 4px;
                }
                .settings-section h2 {
                    margin-top: 0;
                    margin-bottom: 15px;
                    border-bottom: 1px solid var(--text-color);
                    padding-bottom: 10px;
                }
                .palette-container {
                    display: flex;
                    flex-wrap: wrap;
                    gap: 10px;
                    margin-bottom: 20px;
                }
                .color-picker {
                    display: flex;
                    flex-direction: column;
                    align-items: center;
                    gap: 5px;
                }
                .color-block {
                    width: 60px;
                    height: 60px;
                    border: 2px solid var(--text-color);
                    position: relative;
                    cursor: pointer;
                }
                .color-label {
                    font-size: 12px;
                    margin-top: 5px;
                }
                .color-input {
                    opacity: 0;
                    position: absolute;
                    top: 0;
                    left: 0;
                    width: 100%;
                    height: 100%;
                    cursor: pointer;
                }
                .preview-section {
                    margin-top: 20px;
                    display: flex;
                    justify-content: center;
                    flex-direction: column;
                    align-items: center;
                }
                .preview-gradient {
                    width: 100%;
                    height: 60px;
                    background: linear-gradient(to right, #000, #222, #444, #666, #888, #aaa, #ccc, #fff);
                    margin-bottom: 20px;
                }
                .preview-color-values {
                    display: flex;
                    flex-wrap: wrap;
                    gap: 10px;
                    font-size: 12px;
                    margin-top: 10px;
                    width: 100%;
                }
                .preview-color-value {
                    background: rgba(255,255,255,0.1);
                    padding: 5px;
                    border-radius: 3px;
                }
                .notification {
                    position: fixed;
                    top: 20px;
                    right: 20px;
                    padding: 10px 20px;
                    background-color: var(--success-color);
                    color: white;
                    border-radius: 4px;
                    box-shadow: 0 2px 10px rgba(0,0,0,0.2);
                    transform: translateY(-100px);
                    opacity: 0;
                    transition: transform 0.3s, opacity 0.3s;
                }
                .notification.show {
                    transform: translateY(0);
                    opacity: 1;
                }
                @media (max-width: 600px) {
                    .header {
                        flex-direction: column;
                        gap: 10px;
                    }
                    .header-buttons {
                        width: 100%;
                        flex-direction: column;
                    }
                    .button {
                        width: 100%;
                        text-align: center;
                    }
                    .palette-container {
                        justify-content: center;
                    }
                }
            </style>
        </head>
        <body>
            <div class="header">
                <h1>Settings</h1>
                <div class="header-buttons">
                    <button id="resetPaletteBtn" class="button">Reset Palette</button>
                    <button id="savePaletteBtn" class="button button-success">Save Changes</button>
                    <a href="/" class="button">Back to Camera</a>
                    <a href="/gallery" class="button">Gallery</a>
                </div>
            </div>

            <div class="settings-container">
                <div class="settings-section">
                    <h2>Color Palette Settings</h2>
                    <p>Customize the 8-color palette used for image processing:</p>
                    
                    <div class="palette-container" id="palette-container">
                        <!-- Generated by JavaScript -->
                    </div>
                    
                    <div class="preview-section">
                        <h3>Preview</h3>
                        <div class="preview-gradient" id="preview-gradient"></div>
                        <div class="preview-color-values" id="preview-color-values">
                            <!-- Generated by JavaScript -->
                        </div>
                    </div>
                </div>
            </div>
            
            <div id="notification" class="notification">Settings saved successfully!</div>

            <script>
                // Global palette data
                let palette = [
                    {r: 13, g: 43, b: 69, a: 255},  // Dark Blue
                    {r: 32, g: 60, b: 86, a: 255},  // Blue
                    {r: 84, g: 78, b: 104, a: 255}, // Purple
                    {r: 141, g: 105, b: 122, a: 255}, // Lilac
                    {r: 208, g: 129, b: 89, a: 255}, // Orange
                    {r: 255, g: 170, b: 94, a: 255}, // Light Orange
                    {r: 255, g: 212, b: 163, a: 255}, // Beige
                    {r: 255, g: 236, b: 214, a: 255}  // Light Beige
                ];
                
                // Initialize the palette UI
                function initPalette() {
                    const container = document.getElementById('palette-container');
                    container.innerHTML = '';
                    
                    // Create color picker for each palette entry
                    palette.forEach((color, index) => {
                        const colorBlock = document.createElement('div');
                        colorBlock.className = 'color-picker';
                        
                        const block = document.createElement('div');
                        block.className = 'color-block';
                        block.style.backgroundColor = `rgba(${color.r}, ${color.g}, ${color.b}, ${color.a/255})`;
                        
                        const input = document.createElement('input');
                        input.type = 'color';
                        input.className = 'color-input';
                        input.value = rgbToHex(color.r, color.g, color.b);
                        input.dataset.index = index;
                        input.addEventListener('input', handleColorChange);
                         
                        block.appendChild(input);
                        colorBlock.appendChild(block);
                        colorBlock.appendChild(label);
                        container.appendChild(colorBlock);
                    });
                    
                    updatePreviewGradient();
                    updateColorValues();
                }
                
                // Handle color change from color picker
                function handleColorChange(e) {
                    const index = parseInt(e.target.dataset.index);
                    const hexColor = e.target.value;
                    const rgb = hexToRgb(hexColor);
                    
                    palette[index] = {
                        r: rgb.r,
                        g: rgb.g,
                        b: rgb.b,
                        a: 255
                    };
                    
                    // Update the color block
                    const block = e.target.parentElement;
                    block.style.backgroundColor = `rgba(${rgb.r}, ${rgb.g}, ${rgb.b}, 1)`;
                    
                    updatePreviewGradient();
                    updateColorValues();
                }
                
                // Convert RGB to HEX
                function rgbToHex(r, g, b) {
                    return '#' + [r, g, b].map(x => {
                        const hex = x.toString(16);
                        return hex.length === 1 ? '0' + hex : hex;
                    }).join('');
                }
                
                // Convert HEX to RGB
                function hexToRgb(hex) {
                    const result = /^#?([a-f\d]{2})([a-f\d]{2})([a-f\d]{2})$/i.exec(hex);
                    return result ? {
                        r: parseInt(result[1], 16),
                        g: parseInt(result[2], 16),
                        b: parseInt(result[3], 16)
                    } : null;
                }
                
                // Update the preview gradient
                function updatePreviewGradient() {
                    const gradient = document.getElementById('preview-gradient');
                    const stops = palette.map((color, index) => {
                        const percent = (index / (palette.length - 1)) * 100;
                        return `rgba(${color.r}, ${color.g}, ${color.b}, ${color.a/255}) ${percent}%`;
                    }).join(', ');
                    
                    gradient.style.background = `linear-gradient(to right, ${stops})`;
                }
                
                // Update the color values display
                function updateColorValues() {
                    const container = document.getElementById('preview-color-values');
                    container.innerHTML = '';
                    
                    palette.forEach((color, index) => {
                        const div = document.createElement('div');
                        div.className = 'preview-color-value';
                        div.textContent = `${index}: RGB(${color.r}, ${color.g}, ${color.b})`;
                        div.style.color = getBrightness(color) < 128 ? '#fff' : '#000';
                        div.style.backgroundColor = `rgba(${color.r}, ${color.g}, ${color.b}, ${color.a/255})`;
                        container.appendChild(div);
                    });
                }
                
                // Calculate perceived brightness (for text contrast)
                function getBrightness(color) {
                    return Math.sqrt(
                        0.299 * (color.r * color.r) +
                        0.587 * (color.g * color.g) +
                        0.114 * (color.b * color.b)
                    );
                }
                
                // Save the palette to the device
                async function savePalette() {
                    try {
                        const response = await fetch('/save-palette', {
                            method: 'POST',
                            headers: {
                                'Content-Type': 'application/json',
                            },
                            body: JSON.stringify({ palette })
                        });
                        
                        if (response.ok) {
                            showNotification('Settings saved successfully!');
                        } else {
                            showNotification('Failed to save settings', 'error');
                        }
                    } catch (error) {
                        console.error('Error saving palette:', error);
                        showNotification('Error saving settings', 'error');
                    }
                }
                
                // Reset palette to default values
                async function resetPalette() {
                    try {
                        const response = await fetch('/reset-palette', {
                            method: 'POST'
                        });
                        
                        if (response.ok) {
                            // Update the local palette with the response
                            const data = await response.json();
                            if (data.palette) {
                                palette = data.palette;
                                initPalette(); // Refresh UI
                                showNotification('Palette reset to default values');
                            }
                        } else {
                            showNotification('Failed to reset palette', 'error');
                        }
                    } catch (error) {
                        console.error('Error resetting palette:', error);
                        showNotification('Error resetting palette', 'error');
                    }
                }
                
                // Load the current palette from the device
                async function loadPalette() {
                    try {
                        const response = await fetch('/get-palette');
                        if (response.ok) {
                            const data = await response.json();
                            if (data.palette) {
                                palette = data.palette;
                            }
                        }
                    } catch (error) {
                        console.error('Error loading palette:', error);
                    }
                    
                    initPalette();
                }
                
                // Show notification
                function showNotification(message, type = 'success') {
                    const notification = document.getElementById('notification');
                    notification.textContent = message;
                    notification.style.backgroundColor = type === 'success' ? 'var(--success-color)' : 'var(--danger-color)';
                    notification.classList.add('show');
                    
                    setTimeout(() => {
                        notification.classList.remove('show');
                    }, 3000);
                }
                
                // Attach event listeners
                document.addEventListener('DOMContentLoaded', () => {
                    // Load palette first
                    loadPalette();
                    
                    // Setup button event listeners
                    document.getElementById('savePaletteBtn').addEventListener('click', savePalette);
                    document.getElementById('resetPaletteBtn').addEventListener('click', resetPalette);
                });
            </script>
        </body>
        </html>
    )rawliteral";

    return html;
}

#endif // INDEX_HTML_H
