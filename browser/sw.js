"use strict";
const CACHE="wp-field-edition-v1";
const ESSENTIAL=["./","./index.html","./style.css","./game.js","./manifest.webmanifest","./icon.svg"];
self.addEventListener("install",event=>{
  event.waitUntil(caches.open(CACHE).then(cache=>cache.addAll(ESSENTIAL)).then(()=>self.skipWaiting()));
});
self.addEventListener("activate",event=>{
  event.waitUntil(caches.keys().then(names=>Promise.all(names.filter(name=>name.startsWith("wp-field-edition-")&&name!==CACHE).map(name=>caches.delete(name)))).then(()=>self.clients.claim()));
});
self.addEventListener("fetch",event=>{
  if(event.request.method!=="GET"||new URL(event.request.url).origin!==self.location.origin)return;
  event.respondWith(caches.match(event.request).then(hit=>hit||fetch(event.request)));
});
