const char index_html[] PROGMEM = R"rawliteral(
<!doctype html>
<html>
  <head>
      <meta charset="utf-8">
      <meta name="viewport" content="width=device-width,initial-scale=1">
      <title>Raspberry Pi Pico Web Clock ver1.0 </title>
  </head>
  <style>
      html {
        font-family: sans-serif;
      }
      
      h1 {
        color: black;
        font-size: 14px;
        text-align: center;
      }
      
      h2 {
        color: black;
        text-align: center;
      }
      
      canvas {
        width: 10px;
        height: 10px;
        margin: auto;
        border-radius: 5px;
      }
      
	    label {
        display: inline-block;
        width: 80px;
        height: 12px;
        font-size: 14px;
        margin-left: 16px;
      }      

      input {
        width: 95px;
        background: #FFF;
        border-radius: 2px;
        font-size: 14px;
      }      

      input#msg {
        width: 250px;
        height: 10px;
        padding: 3px;
        margin: auto;
        background: #FFF;
        border-radius: 2px;
      }      

      textarea {
        width: 405px;
        min-height: 10px;
        box-shadow: 5px 5px 10px rgba(0,0,0,0.7);
        padding: 10px;
        background: #FFF;
        border-radius: 7px;
        margin-top: 20px;
        font-size: 14px;
     }
      
      logContainer {
        width: 405px;
        min-height: 300px;
        max-height: 500px;
        font-size: 13px;
        margin: auto;
        padding: 10px;
        background: #FFF;
        box-shadow: 5px 5px 10px rgba(0,0,0,0.7);
        border-radius: 10px;
        line-height: 1.0;
        white-space: pre;
        overflow: scroll;
        display: none;
      }
      
      #menu {
          display: block;
          flex-direction: column;
          flex-wrap: nowrap;
          width: 430px;
          min-height: 510px;
          max-height: 400px;
          background: #CCC;
          padding: 8px;
          border-radius: 8px;
          margin-top: 10px;
          margin-right: 10px;
      }

      section#Message {
          display: flex;
          flex-wrap: nowrap;
          background: #CCE;
          align-items: center;
      }

      section#CheckBox {
          display: flex;
          background: #CCE;
          line-height: 25px;
          margin-top: 10px;
          flex-wrap: nowrap;
      }

      .cb {
        display: inline-block;
        line-height: 25px;
        margin-top: 8px;
        width: 20px;
        height: 15px;
	    }
      
      .cbox {
        margin-top: 3px;
        display: flex;
        width: 10px;
	    }
      
      section#Params {
          display: inline-block;
          margin-top: 20px;
          background: #CCE;
          flex-wrap: nowrap;
      }

	    .timeLabel {
        display: inline-block;
        width: 40px;
        height: 12px;
        font-size: 14px;
        margin-left: 6px;
      }
      
	    .devtime {
        display: inline-block;
        width: 400px;
        height: 12px;
        font-size: 14px;
        margin-left: 6px;
      }
      
      button {
          display: inline-block;
          margin: 4px;
          padding: 0 8px;
          border: 0;
          line-height: 25px;
          cursor: pointer;
          color: #fff;
          background: #A0A0A0;
          border-radius: 5px;
          font-size: 14px;
      }

      button:hover {
          background: #ff494d
      }

  </style>

    <h2>Raspberry Pi Web Clock v1.0</h2>
    <h1>Copyright&#169 2022 Denubian Designs</h1>
    <hr />
    <div id="content">
        <nav id="menu">
          <section id="Message">
            <button id="sendMsg">Send Msg</button>
            <input  id="msg" type="text" maxlength="30" placeholder="Msg max 30 Chars" required>
            <canvas id="HeartBeat"width="10" height="10" style="border:1px solid #d3d3d3;"></canvas>
          </section>
          <section id="CheckBox">
            <button id="playSnd">Play Sound</button>
            <label class="cbox">1:</label>
  			    <input class="cb" id="yes" name="buttons" type="radio" value="1">
            <label class="cbox">2:</label>
 		        <input class="cb" id="no" name="buttons" type="radio" value="2">
            <label class="cbox">3:</label>
		        <input class="cb" id="ok" name="buttons" type="radio" value="3">
            <label class="cbox">4:</label>
		        <input class="cb" id="ok" name="buttons" type="radio" value="4">
            <label class="cbox">5:</label>
		        <input class="cb" id="ok" name="buttons" type="radio" value="5">
            <label class="cbox">6:</label>
		        <input class="cb" id="ok" name="buttons" type="radio" value="6">
          </section>
          <section id="Params">
            <button id="setAlarm">Set Alarm</button>
            <label class="timeLabel" for="AlarmOn">On:</label>
            <input type="time" id="AlarmOn">
            <label class="timeLabel" for="AlarmOff">Off:</label>
            <input type="time" id="AlarmOff">
            <p>
            <label class="devtime" id="rPiTime"> Device Time </label>
          </section>
          <section id="">
             <textarea id="statusArea" rows="2" cols="50"> </textarea>
          </section>
          <logContainer>
          </logContainer>
          </nav>
    </div>
    
  <script>

  var gateway = `ws://${window.location.hostname}/ws`;
  var websocket;
  var param;
  var x;
  
  window.addEventListener('load', onLoad);
  
  function initWebSocket() {
    console.log('Trying to open a WebSocket connection...');
    websocket = new WebSocket(gateway);
    websocket.onopen    = onOpen;
    websocket.onclose   = onClose;
    websocket.onmessage = onMessage;
  }

  function initButtons() {
    document.getElementById('sendMsg').addEventListener('click', sendMsg);
    document.getElementById('playSnd').addEventListener('click', playSound);
    document.getElementById('setAlarm').addEventListener('click', setAlarm);
  }

  function sendMsg() {
    param = document.getElementById('msg').value;
    if(param == '') {
      window.alert('Message cannot be blank');
    } else {
      console.log('putMsg,' + param);
      websocket.send('putMsg,' + param);
    }
  }
  
  function playSound() {
    param = document.querySelector('input[name="buttons"]:checked').value;
    console.log('playSound,' + param);
    websocket.send('playSound,' + param);
  }
  
  function setAlarm() {
    if(document.getElementById('AlarmOn').value == 0 ||
         document.getElementById('AlarmOff').value == 0 ||
            document.getElementById('AlarmOn').value > document.getElementById('AlarmOff').value) {
      window.alert('Invalid Time, click the "Clock" icon to select a valid time');
    } else {
      param = document.getElementById('AlarmOn').value;
      param = param + '#' + document.getElementById('AlarmOff').value;
    }
    console.log('setAlarm,' + param);
    websocket.send('setAlarm,' + param);
  }
  
  function onOpen(event) {
    console.log('Connection opened');
  }
  
  function onClose(event) {
    console.log('Connection closed');
    setTimeout(initWebSocket, 2000);
  }
  
  function onMessage(event) {
    console.log(event.data);
    const cmd = event.data.split(';');
    
    if (cmd[0] == 'Status'){
      ToggleHeartBeat();
      document.getElementById('rPiTime').innerHTML = cmd[1];  //display the current time
    } else if (cmd[0] == 'Response'){
      document.getElementById('statusArea').innerHTML = cmd[1];

      if (cmd[1].includes('Response to Message Received')){
        document.getElementById('msg').value = '';
        document.querySelector('input[name="buttons"]:checked').checked = false;
      }
    } else if (cmd[0] == 'Alarm'){
      if(cmd[1] == "On") {
        NotifyUser();  //Notify user on the PC
      } else {
        ResetAlarm();  //Reset the user alarm
      }
    }
  }
  
  function ResetAlarm() {
    document.getElementById('statusArea').style.backgroundColor = "#DDD";
  }
  
  function NotifyUser() {
    document.getElementById('statusArea').style.backgroundColor = "red";
  }
 
  function getPiStatus() {              //Happens once every second so check for button presses as well
    if(websocket.readyState == 1) {
      websocket.send('getPiStatus');
    }
  }
 
  function ToggleHeartBeat() {
    var hb = document.getElementById("HeartBeat");
    var ctx = hb.getContext("2d");
  
    if (x == 0) {
      ctx.fillStyle = "#FF0000";
      x = 1;
    } else {
      ctx.fillStyle = "#FFF";
      x = 0;
    }
    ctx.fillRect(0, 0, 10, 10);  
  }

  function onLoad(event) {
    initWebSocket();
    initButtons();
    setInterval(getPiStatus, 1000);
  }

</script>
</body>
</html>
)rawliteral";
