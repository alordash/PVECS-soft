const int cSize = sizeof(char);
// Index page
const char IndexPage[] PROGMEM
	= R"=====(<!DOCTYPE html><html><head><meta http-equiv="Content-Type" content="text/html; charset=UTF-8"><title>Energy Consumption Tracker</title><style type="text/css"> buttonOne { text-align: center; font-size: 10vmin; border: 2vmin double #D8DFEA; border-radius: 200px 40px 200px 40px; padding: 3vmin; margin: auto; padding-left: 6vmin; padding-right: 6vmin; display: inline-block; width: 55vmin} a { text-decoration: none; } buttonOne:hover { border: 2vmin double #3B5998; box-shadow: 0 5px 15px #000000; color: #551A8B; background-color: azure; }</style></head><body vlink="#262E45" link="#3030A5" alink="#339988"><div style="text-align: center;margin-bottom: 10px;"><buttonone style="word-wrap: break-word;"><a href="http://192.168.1.40/LEDControl">LED Control</a></buttonone></div><div style="text-align: center;margin-bottom: 10px;"><buttonone style="border-radius: 40px 200px 40px 200px;"><a href="http://192.168.1.40/WiFiSettings">WiFi Settings</a></buttonone></div><div style="text-align: center;"><buttonone style=""><a href="http://192.168.1.40/ObtainedData">Data Table</a></buttonone></div></body></html>)=====";

const int IndexPageSize = sizeof(IndexPage);
// SettingsPage
const char SettingsPageBegin[] PROGMEM = R"=====(
<!DOCTYPE html>
<html><head><meta http-equiv="Content-Type" content="text/html; charset=windows-1252">
<title>ECT: LED Control</title>
<style>
.mainPanel{
    border: solid;
    width: 50vw;
    height: 300px;
    padding: 10px;
}
.lightDisplay{
    margin: auto;
    margin-top: 8px;
    margin-bottom: 4px;
    height: 20px;
    width: 27vw;
    background: linear-gradient(90deg, rgba(0,255,0,1) 0%, rgba(255,255,0,1) 50%, rgba(255,0,0,1) 100%);
    border: solid;
    border-color: #6457c1;
    border-left: solid;
    border-left-width: 2vw;
    border-left-color: #6457c1;
    border-right: solid;
    border-right-width: 2vw;
    border-right-color: #6457c1;
    border-radius: 15px;
    align-content: center;
}
.textBox {
	width: 31px;
	height: 18px;
	border: solid;
	border-color: #898989;
    border-width: 2px;
	border-radius: 7px;
    background-color: #d0fff0;
    box-shadow: inset 0px 0px 2px 0px black;
	padding: 2px;
	margin: 10px;
	outline: none;
    margin-left:28px;
    position: absolute;
}
.textBox:focus {
	border: solid;
	border-width: 2px;
	border-radius: 7px;
	background-color: #aecad2;
	box-shadow: inset 0px 0px 2px 0px black;
}
.textBox:hover{
	border: solid;
	border-width: 2px;
	border-radius: 7px;
}
.submitBox{
    position: absolute;
    outline: none;
    height: 44px;
    width: 144px;
    background-color: #856c9666;
    border: solid;
	border-color: #ffffff;
	border-width: 2px;
    border-radius: 100px;
    box-shadow: inset 0 0 4px 0px #ffffff;
}
.submitBox:active{
  background-color: #333;
  border-color: #333;
  color: #eee;
  box-shadow: inset 0 0 8px 0px #ffffff;
}
.submitBox:hover{
    border: solid;
    border-width: 2px;
    border-radius: 100px;
}
.modeText {
    position: relative;
    text-align: center;
    margin: auto;
}
.boundText {
    position: absolute;
    font-size: 12px;
    background-color: #ffffff;
}
</style>
</head>
<body>
<div name="Control Panel" class="mainPanel" style="margin: auto;">
    <p style="margin: auto; text-align: center;">LED Light Display</p>
    <div name="LED Light Display" class="lightDisplay">
        <form action="/getParam" style="position: relative;">
)=====";

const char SettingsLeftBorderBegin[] PROGMEM = R"=====(<input type="text" maxlength = "4" class="textBox" name="LW)=====";
const char SettingsLeftBorderMiddle[] PROGMEM = R"=====(" style="top: )=====";
const char SettingsLeftBorderEnd[] PROGMEM = R"=====(px; left: -40px;" value = ")=====";
const char SettingsLeftBordeClose[] PROGMEM = R"=====(">)=====";

const char SettingsRightBorderBegin[] PROGMEM = R"=====(<input type="text" maxlength = "4" class="textBox" name="UB)=====";
const char SettingsRightBorderMiddle[] PROGMEM = R"=====(" style="top: )=====";
const char SettingsRightBorderEnd[] PROGMEM = R"=====(px; right: -23px;" value = ")=====";
const char SettingsRightBordeClose[] PROGMEM = R"=====(">)=====";

const char SettingsModeBegin[] PROGMEM = R"=====(<p class="modeText" style="top: )=====";
const char SettingsModeMiddle[] PROGMEM = R"=====(px;">Mode )=====";
const char SettingsModeEnd[] PROGMEM = R"=====(</p>)=====";

const char SettingsLeftBoundBegin[] PROGMEM = R"=====(<a style="left: -49px; top: )=====";
const char SettingsLeftBoundEnd[] PROGMEM = R"=====(px;" class="boundText">Lower<br>Bound</a>)=====";
const char SettingsRightBoundBegin[] PROGMEM = R"=====(<a style="right: -49px; top: )=====";
const char SettingsRightBoundEnd[] PROGMEM = R"=====(px;" class="boundText">Upper<br>Bound</a>)=====";

const char SettingsPageEnd[] PROGMEM = R"=====(
<div style="position: relative; float: right; right: 50%;">
                <input type="submit" class="submitBox" value="Submit" style="position: relative; float: left; left: 50%; top: 108px;">
            </div>
        </form>
    </div>
</div>
	<div style="position: relative; float: left; left: 50%; margin-top: 10px;">
		<form action = "/index">
			<input type="submit" value="Return" style="height: 38px; position: relative; float: right; right: 50%;">
		</form>
	</div>
</body></html>)=====";

const int settingsPageSize = sizeof(SettingsPageBegin) + sizeof(SettingsPageEnd) + sizeof(SettingsLeftBorderBegin) * 4 + sizeof(SettingsLeftBorderEnd) * 4
							 + sizeof(SettingsRightBorderBegin) * 4 + sizeof(SettingsRightBorderEnd) * 4 + sizeof(SettingsModeBegin) * 4
							 + sizeof(SettingsModeMiddle) * 4 + sizeof(SettingsModeEnd) * 4 + sizeof(SettingsLeftBoundBegin) * 4
							 + sizeof(SettingsLeftBoundEnd) * 4 + sizeof(SettingsRightBoundBegin) * 4 + sizeof(SettingsRightBoundEnd) * 4
							 + sizeof(SettingsPageEnd) + 100;

// WiFi networks page
const char WiFiNetPageBegin[] PROGMEM = R"=====(<!DOCTYPE html>
<html><head><meta http-equiv="Content-Type" content="text/html; charset=windows-1252">
<title>ECT: WiFi Settings</title>
<style>
.mainPanel{
    border: solid;
    width: 50vw;
    height: 300px;
    padding: 10px;
}
.textBox {
	width: 150px;
	float: right;
}
</style>
</head>
<body>
	<div style = "padding-left: 10px; padding-right: 10px; border: ridge; border-color: darkgrey; width: 320px;">
		<p><u>Add WiFi net</u></p>
		<form action="/saveWiFiNet">
			<p><b>SSID</b><input type="text" maxlength="32" class = "textBox" name = "SSID" style = "height: 1em; width: 18em;"></p>
			<p><b>Password</b><input type="text" maxlength="32" class = "textBox" name = "pass" style = "height: 1em; width: 18em;"></p>
			<input type="submit" value="Add" style="margin-bottom: 10px; left: 280px; position: relative;">
		</form>
        <form action="/index" style="position: absolute;">
			<input type="submit" value="Return" style="height: 28px; bottom: 10px; position: absolute;">
		</form>)=====";
const char WiFiNetPageStoreError[] PROGMEM = R"=====(<strong><p style="color: red; margin-bottom: 10px;">Error: too many stored networks</p></strong>)=====";
const char WiFiNetPageBegin2[] PROGMEM = R"=====(</div>
	<div style = "margin-top: 10px; padding: 10px; border: ridge; border-color: darkgrey; width: 320px;">
		<p><u>Delete WiFi net</u></p>
		<form action="/delWiFiNet">
			<p>List of stored networks:</p>)=====";
const char WiFiNetEmptyList[] PROGMEM = R"=====(<strong><p>Empty</p></strong>)=====";
const char WiFiNetListBegin[] PROGMEM = R"=====(<button type="submit" name ="name" value=")=====";
const char WiFiNetListMiddle[] PROGMEM = R"=====(" style="margin-right: 10px; margin-bottom: 10px;">Delete</button><b>)=====";
const char WiFiNetListEnd[] PROGMEM = R"=====(</b>
			<br>)=====";
const char WiFiNetPageEnd[] PROGMEM = R"=====(		</form>
	</div>
    <p>Connected net: <strong>)=====";
const char WiFiNetPageClose[] PROGMEM = R"=====(</strong></p></body></html>)=====";

const int WiFiNetPageSize = sizeof(WiFiNetPageBegin) + sizeof(WiFiNetPageStoreError) + sizeof(WiFiNetPageBegin2) + sizeof(WiFiNetEmptyList)
							+ 10 * (sizeof(WiFiNetListBegin) + sizeof(WiFiNetListMiddle) + sizeof(WiFiNetListEnd)) + sizeof(WiFiNetPageEnd)
							+ sizeof(WiFiNetPageClose) + 30;

// DataObtained page
const char DataObtainedPageBegin[] PROGMEM = R"=====(<!DOCTYPE html>
<html><head>
<meta http-equiv="content-type" content="text/html; charset=UTF-8">
  <meta charset="utf-8">
  <title>ECT: Data Obtained</title>
<style>.time {
  background: #ffff0025;
  }
.power {
  background: #ff653635;
  }
.CO2 {
  background: #1ce10035;
  }
.temperature {
  background: #009fff35;
  }</style>
 </head>
 <body>
  <form action="/index">
			<input type="submit" value="Return" style="height: 28px;">
		</form>
  <table border="1">
   <caption>Sensor data, page: )=====";
const char DataObtainedPageStyles[] PROGMEM = R"=====(</caption>
   <tbody><tr>
    <th style="background: #ffff00;">Time</th>
    <th style="background: #ff6536;">Power</th>
    <th style="background: #1ce100;">CO2</th>
    <th style="background: #009fff;">Temperature</th>
   </tr>)=====";
const char DataObtainedTime[] PROGMEM = R"=====(<tr><td>)=====";
const char DataObtainedTimeLong[] PROGMEM = R"=====(&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;)=====";
const char DataObtainedTimeShort[] PROGMEM = R"=====(&nbsp;&nbsp;)=====";
const char DataObtainedPower[] PROGMEM = R"=====(</td><td class="power">)=====";
const char DataObtainedCO2[] PROGMEM = R"=====(</td><td class="CO2">)=====";
const char DataObtainedTemperature[] PROGMEM = R"=====(</td><td class="temperature">)=====";
const char DataObtainedRowEnd[] PROGMEM = R"=====(</td></tr>)=====";
const char DataObtainedPageEnd[] PROGMEM = R"=====(
  </tbody></table> 
  <form style="position: relative;" action="/ObtainedDataBack">
		<input type="submit" value="Back" style="height: 28px;width: 70px;">
  </form>
  <form style="position: relative;top: -28px;left: 75px;" action="/ObtainedDataForward">
		<input type="submit" value="Forward" style="height: 28px;width: 70px;"></form>
</body></html>)=====";
const int DataObtainedMaxSamples = 7 * 4;
const int DataObtainedSampleSize = sizeof(DataObtainedTime) + sizeof(DataObtainedTimeLong) + cSize * 21 + sizeof(DataObtainedPower) + cSize * 9
								   + sizeof(DataObtainedCO2) + cSize * 9 + sizeof(DataObtainedTemperature) + cSize * 9 + sizeof(DataObtainedRowEnd);
const int DataObtainedPageSize
	= sizeof(DataObtainedPageBegin) + cSize * 9 + sizeof(DataObtainedPageStyles) + sizeof(DataObtainedPageEnd) + DataObtainedMaxSamples * DataObtainedSampleSize;