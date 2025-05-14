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

#endif // INDEX_HTML_H
