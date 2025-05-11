#ifndef INDEX_HTML_H
#define INDEX_HTML_H


const char* getIndexHTML() {
    static const char html[] = R"rawliteral(
        <!DOCTYPE html><html lang="en"><meta charset="UTF-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>Shakal Cam</title><link rel="icon" type="image/png" id="favicon" href="#"><style>:root{--font-size:24px;--background-color:#000;--text-color:#fff}body,html{width:100%;height:100%;margin:0;padding:0;background-color:var(--background-color);font-family:monospace}.container{width:100%;height:100%;display:flex;justify-content:center;align-items:center;position:relative}.controls{position:absolute;bottom:20px;left:20px;right:20px;display:flex;justify-content:space-between;align-items:center;gap:1rem}.controls__group{display:flex;align-items:center;gap:1rem}.time-elapsed{color:var(--text-color);font-size:var(--font-size);font-weight:700;margin-left:10px}.button{background-color:var(--text-color);color:var(--background-color);border:none;padding:10px 20px;font-size:var(--font-size);font-weight:700;cursor:pointer;font-family:monospace;box-shadow:5px 5px 0 0 var(--background-color)/50%}.button:active{box-shadow:0 0 0 0 var(--background-color)/50%;transform:translate(5px,5px)}.image{width:100%;height:100%;aspect-ratio:1.5;object-fit:cover}@media (max-width:768px){.controls{top:10px;bottom:10px;flex-direction:column;--font-size:18px}}</style><div class="container"><img src="#" width="640" height="480" class="image js-image" onload=""><div class="controls"><div class="controls__group"><span class="js-time-elapsed time-elapsed">00:00</span> <span class="js-fps time-elapsed">0 FPS</span></div><div class="controls__group"><button class="button js-flashlight-button">Flash off</button> <button class="button js-control-button">Stop</button> <button class="button js-capture-button">Capture</button></div></div></div><script>const INTERVAL_MS=1e3;let currentHost=window.location.host,isPaused=!1,isFlashOn=!1,dataUrl=null;const controlButton=document.querySelector(".js-control-button"),captureButton=document.querySelector(".js-capture-button"),flashlightButton=document.querySelector(".js-flashlight-button"),image=document.querySelector(".js-image"),timeElapsedElement=document.querySelector(".js-time-elapsed"),fpsElement=document.querySelector(".js-fps");function updateImage(){const t=new Date,e=`http://${currentHost}/capture`;image.src=`${e}?t=${t.getTime()}`,image.onload=()=>{image.style.opacity=1;const e=(new Date).getTime()-t;timeElapsedElement.innerText=`${e}ms`;const a=1e3/e;fpsElement.innerText=`${a.toFixed(2)} FPS`,captureImage(image)}}function flash(){return new Promise(((t,e)=>{fetch(`http://${currentHost}/flash`).then((()=>{setTimeout((()=>{fetch(`http://${currentHost}/flash`)}),300)})),setTimeout((()=>{t()}),200)}))}function captureImage(t){const e=document.createElement("canvas");e.width=t.naturalWidth,e.height=t.naturalHeight;e.getContext("2d").drawImage(t,0,0),dataUrl=e.toDataURL("image/png"),localStorage.setItem("lastimage",dataUrl);const a=document.createElement("canvas");a.width=32,a.height=32;a.getContext("2d").drawImage(t,0,0,32,32);const n=a.toDataURL("image/png");document.getElementById("favicon").href=n}localStorage.getItem("lastimage")&&(image.src=localStorage.getItem("lastimage"),image.style.opacity=.5),controlButton.addEventListener("click",(()=>{isPaused=!isPaused,controlButton.innerText=isPaused?"Resume":"Pause"})),captureButton.addEventListener("click",(async()=>{if(!dataUrl)return;isPaused=!0,isFlashOn&&await flash(),updateImage();const t=document.createElement("a");t.href=dataUrl,t.download=`capture_${(new Date).toISOString()}.png`,document.body.appendChild(t),t.click(),document.body.removeChild(t),isPaused=!1})),flashlightButton.addEventListener("click",(()=>{isFlashOn?(isFlashOn=!1,flashlightButton.innerText="Flash off"):(isFlashOn=!0,flashlightButton.innerText="Flash on")})),setInterval((()=>{isPaused||updateImage()}),1e3)</script>
    )rawliteral";

    // static const char html[] = R"rawliteral(
    //     <!DOCTYPE html>
    //     <html lang="en">
    //     <head>
    //         <meta charset="UTF-8">
    //         <title>Simple Page</title>
    //     </head>
    //     <body>
    //         <h1>Hello, World!</h1>
    //         <p>This is a simple HTML page.</p>
    //     </body>
    //     </html>
    // )rawliteral";
    return html;
}

#endif // INDEX_HTML_H
