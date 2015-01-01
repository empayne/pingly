var TIMEOUT = 10000;
var SITE_TO_PING = "http://www.google.com";

// Listen for when the watchface is opened:
Pebble.addEventListener('ready', 
  function(e) {
    console.log("PebbleKit JS ready!");
  }
);

// Listen for when an AppMessage is received:
Pebble.addEventListener('appmessage',
  function(e) {
    console.log("AppMessage received!");
    ping(SITE_TO_PING, TIMEOUT, send_latency_data);
  }                     
);

// Use an XHR request for an image that doesn't exist. If we get a 404 / most other error codes, we're connected.
// We get an error code of 0 if we don't have a network / we time out.
// To test a timeout, we can try accessing a website with a long timeout period, like: "http://google.com:81/"
// Adapted from: http://forums.getpebble.com/discussion/15547/timeout-on-xmlhttprequest
function ping(site, timeout, callback) {
    this.xhr = new XMLHttpRequest();
    this.file = site + "/fakeimage.png/?cachebreaker=" + new Date().getTime();
  
    console.log("Setting up XHR...");
    xhr.timeout = timeout;
    xhr.ontimeout = function () {
      // If we time out, onreadystatechanged is also called with status 0 (which then sends message to Pebble).
      console.log("XHR timed out."); 
    };
  
    xhr.onreadystatechange = function () {
      if (xhr.readyState == 4) // Response is ready:
        {
          this.latency = (new Date().getTime()) - start;
          console.log("Received XHR response (status " + xhr.status + ") after " + this.latency + " ms.");
          callback(this.latency, xhr.status != 0);
        }
    };
  
    console.log("Open XHR...");
    xhr.open('HEAD', file, true);
   
    console.log("Send XHR...");
    this.start = new Date().getTime();
    xhr.send();
  
    console.log("XHR sent.");
}

// Send data from the ping function to the Pebble.
function send_latency_data(latency, success) {
  console.log("Sending latency data...");
  
  this.connected = success ? 1 : 0;
  this.latency = latency;
  this.dictionary = null;
  
  // Log results:
  console.log ("Done pinging: " + (connected ? "connected to" : "disconnected from") + " internet.");
  if (connected != 0) {
    console.log("Latency is " + latency + "ms.");
  }
  
  dictionary = {
    "KEY_CONNECTED": connected,
    "KEY_LATENCY": latency
  };
  
  Pebble.sendAppMessage(dictionary,
    function(e) {
      console.log("Latency info sent to Pebble successfully!");
    },
    function(e) {
      console.log("Error sending latency info to Pebble!");
    }
  );
}
