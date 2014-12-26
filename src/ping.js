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
    ping(SITE_TO_PING);
  }                     
);

// Use an image request to test if we can connect to the server.
// This doesn't really work, see commit history for explanation.
// Adapted from: http://stackoverflow.com/questions/4282151/is-it-possible-to-ping-a-server-from-javascript
function ping(SITE_TO_PING) {
    console.log("in pinger ping");
    var pinger = this;
    this.latency = -1;
  
    this.img = new Image();
  
    // TODO: don't send latency data if we've already timed out.
    this.img.onload = function() {
      console.log("Successful ping: in onload.");
      pinger.latency = (new Date().getTime()) - pinger.start;
      send_latency_data(pinger.latency, true);
    };
  
    this.img.onerror = function() {
      console.log("Successful ping: in onerror.");
      pinger.latency = (new Date().getTime()) - pinger.start;
      send_latency_data(pinger.latency, true);
    };

    this.start = new Date().getTime();
    this.img.src = SITE_TO_PING + "/?cachebreaker=" + new Date().getTime();
    
    this.timer = setTimeout(function() {
      console.log("Entering timeout...");
      if (this.latency == -1) {
        console.log("Unsuccessful ping: timed out after " + TIMEOUT + " ms.");
        send_latency_data(pinger.latency, false);
      }
      else {
        console.log("Ping was successful prior to the timeout.");
      }
    }, TIMEOUT);
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
