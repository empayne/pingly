var sitesToPing = ["http://www.google.com", "http://www.yahoo.com"];

// Main ping function:
function ping(sitesToPing) {
  var connected;
  var latency;
  var dictionary;
  
  // Ping the websites:
  // TODO: ping the websites
  connected = 1;
  latency = 10;  
  
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

// Listen for when the watchface is opened
Pebble.addEventListener('ready', 
  function(e) {
    console.log("PebbleKit JS ready!");
  }
);

// Listen for when an AppMessage is received
Pebble.addEventListener('appmessage',
  function(e) {
    console.log("AppMessage received!");
    ping(sitesToPing);
  }                     
);