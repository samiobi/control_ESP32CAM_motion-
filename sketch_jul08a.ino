#include <ESP8266WiFi.h>
#include <Servo.h>

const char* ssid = "XXXXXX";
const char* password = "XXXXX";

Servo servoUpDown;       // Vertical servo
Servo servoLeftRight;    // Horizontal servo

int angleUpDown = 90;      // Center position
int angleLeftRight = 90;

WiFiServer server(80);

void setup() {
  Serial.begin(115200);

  servoUpDown.attach(D5);      // GPIO14
  servoLeftRight.attach(D6);   // GPIO12

  servoUpDown.write(angleUpDown);
  servoLeftRight.write(angleLeftRight);

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }
  Serial.println("\nConnected! IP:");
  Serial.println(WiFi.localIP());

  server.begin();
}

void loop() {
  WiFiClient client = server.available();
  if (client) {
    String request = client.readStringUntil('\r');
    client.flush();

    if (request.indexOf("GET /control?cmd=") != -1) {
      if (request.indexOf("up") != -1) angleUpDown = constrain(angleUpDown - 5, 0, 180);
      if (request.indexOf("down") != -1) angleUpDown = constrain(angleUpDown + 5, 0, 180);
      if (request.indexOf("left") != -1) angleLeftRight = constrain(angleLeftRight + 5, 0, 180);
      if (request.indexOf("right") != -1) angleLeftRight = constrain(angleLeftRight - 5, 0, 180);

      if (request.indexOf("center") != -1) {
        angleUpDown = 90;
        angleLeftRight = 90;
      }

      servoUpDown.write(angleUpDown);
      servoLeftRight.write(angleLeftRight);

      int x = map(angleLeftRight, 0, 180, 210, 0);
      int y = map(angleUpDown, 0, 180, 0, 210);

      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: application/json");
      client.println("Connection: close");
      client.println();
      client.print("{\"x\":");
      client.print(x);
      client.print(",\"y\":");
      client.print(y);
      client.println("}");
      return;
    }

    // Send HTML Page
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html");
    client.println("Connection: close");
    client.println();
    client.println("<!DOCTYPE html><html><head><meta charset='UTF-8'><title>Servo Control</title>");
    client.println("<style>");
    client.println("body{text-align:center;font-family:sans-serif;}");
    client.println(".btn{padding:10px 20px;margin:5px;font-size:18px;}");
    client.println("#grid{width:220px;height:220px;margin:auto;border:2px solid #000;position:relative;background:#f0f0f0;}");
    client.println("#dot{width:10px;height:10px;background:red;border-radius:50%;position:absolute;transition:top 0.2s,left 0.2s;}");
    client.println(".axis-x{position:absolute;top:50%;left:0;width:100%;height:1px;background:black;}");
    client.println(".axis-y{position:absolute;left:50%;top:0;height:100%;width:1px;background:black;}");
    client.println("</style>");
    client.println("</head><body>");

    client.println("<h2>Platform Control Panel</h2>");

    // Button grid layout with data-cmd for each
    client.println("<div style='display:grid;grid-template-columns:1fr 1fr 1fr;width:240px;margin:auto;'>");
    client.println("<div></div><button class='btn control' data-cmd='up'>UP</button><div></div>");
    client.println("<button class='btn control' data-cmd='left'>LEFT</button>");
    client.println("<button class='btn' onclick=\"sendOnce('center')\">CENTER</button>");
    client.println("<button class='btn control' data-cmd='right'>RIGHT</button>");
    client.println("<div></div><button class='btn control' data-cmd='down'>DOWN</button><div></div>");
    client.println("</div>");

    client.println("<h3>Position: <span id='posText'>X=90°, Y=90°</span></h3>");
    client.println("<div id='grid'>");
    client.println("<div class='axis-x'></div><div class='axis-y'></div>");
    client.println("<div id='dot' style='top:105px; left:105px;'></div>");
    client.println("</div>");

    // JavaScript section
    client.println("<script>");
    client.println("let interval = null;");
    client.println("function sendCmd(cmd){");
    client.println(" fetch('/control?cmd=' + cmd).then(r => r.json()).then(updateDot);");
    client.println("}");
    client.println("function sendOnce(cmd){ sendCmd(cmd); }");
    client.println("function updateDot(data){");
    client.println(" document.getElementById('dot').style.left = data.x + 'px';");
    client.println(" document.getElementById('dot').style.top = data.y + 'px';");
    client.println(" document.getElementById('posText').innerText = 'X=' + Math.round(data.x * 180 / 210) + '°, Y=' + Math.round(data.y * 180 / 210) + '°';");
    client.println("}");

    // Universal input via pointer events (touch + mouse)
    client.println("document.querySelectorAll('.control').forEach(btn => {");
    client.println(" let cmd = btn.dataset.cmd;");
    client.println(" btn.addEventListener('pointerdown', e => {");
    client.println("   e.preventDefault();");
    client.println("   if(interval) clearInterval(interval);");
    client.println("   sendCmd(cmd);");
    client.println("   interval = setInterval(() => sendCmd(cmd), 150);");
    client.println(" });");
    client.println(" btn.addEventListener('pointerup', () => { clearInterval(interval); });");
    client.println(" btn.addEventListener('pointerleave', () => { clearInterval(interval); });");
    client.println("});");
    client.println("window.addEventListener('pointerup', () => { clearInterval(interval); });");
    client.println("</script>");

    client.println("</body></html>");
    client.stop();
  }
}
