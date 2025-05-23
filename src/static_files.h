#pragma once
const char AP_SETTINGS_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="ru">
<head> 
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">

    <title>Настройка WiFi для ESP32-CAM</title>
    <style> 
        body { font-family: Arial, sans-serif; background: #f0f0f0; }
        .container { max-width: 600px; margin: 20px auto; background: #fff; border-radius: 10px; box-shadow: 0 2px 8px #aaa; padding: 30px; }
        .wifi-table { width: 100%; border-collapse: separate; border-spacing: 20px 10px; }
        .wifi-table th, .wifi-table td { text-align: left; vertical-align: top; }
        label { display: block; margin-bottom: 6px; color: #555; }
        input[type="text"], input[type="password"] {
            width: 100%; padding: 10px; border: 1px solid #ccc; border-radius: 5px;
            font-size: 16px; box-sizing: border-box;
        }
        input[type="submit"] {
            width: 100%; padding: 12px; background: #4CAF50; color: #fff;
            border: none; border-radius: 5px; font-size: 18px; margin-top: 20px; cursor: pointer;
            transition: background 0.2s;
        }
        input[type="submit"]:hover { background: #388e3c; }
        .info { margin-top: 20px; color: #888; font-size: 14px; text-align: center; }
        .main-title {
            text-align: center;
            color: #333;
            margin: 40px auto 0 auto;
            font-size: 24px;
            max-width: 600px;
        }
        .footer {
            text-align: center;
            margin-top: 30px;
            color: #666;
            font-size: 14px;
        }
        @media (max-width: 600px) {
            body {
                padding: 10px;
            }
            .container {
                padding: 15px;
            }
            .main-title {
                font-size: 20px;
                margin-top: 20px;
            }
            .wifi-table {
                border-spacing: 10px 5px;
            }
            .wifi-table th, .wifi-table td {
                display: block;
                width: 100%;
                box-sizing: border-box;
            }
            .wifi-table tr {
                display: block;
                margin-bottom: 10px;
            }
            input[type="text"], input[type="password"] {
                padding: 10px;
                font-size: 15px;
            }
            input[type="submit"] {
                padding: 12px;
                font-size: 16px;
                height: auto;
            }
            .footer {
                font-size: 13px;
                margin-top: 30px;
            }
        }
    </style>
</head>
<body>
    <h1 class="main-title">Настройка WiFi для ESP32-CAM</h1>
    <div class="container">
        <form method="POST" action="/save">
            <table class="wifi-table">
                <tr>
                    <td>
                        <label for="ssid1">WiFi 1:</label>
                        <input type="text" id="ssid1" name="ssid1" required placeholder="Введите SSID WiFi 1">
                    </td>
                    <td>
                        <label for="password1">&nbsp;</label>
                        <input type="password" id="password1" name="password1" required placeholder="Введите пароль WiFi 1">
                    </td>
                </tr>
                <tr>
                    <td>
                        <label for="ssid2">WiFi 2:</label>
                        <input type="text" id="ssid2" name="ssid2" placeholder="Введите SSID WiFi 2">
                    </td>
                    <td>
                        <label for="password2">&nbsp;</label>
                        <input type="password" id="password2" name="password2" placeholder="Введите пароль WiFi 2">
                    </td>
                </tr>
                <tr>
                    <td>
                        <label for="ssid3">WiFi 3:</label>
                        <input type="text" id="ssid3" name="ssid3" placeholder="Введите SSID WiFi 3">
                    </td>
                    <td>
                        <label for="password3">&nbsp;</label>
                        <input type="password" id="password3" name="password3" placeholder="Введите пароль WiFi 3">
                    </td>
                </tr>
            </table>
            <input type="submit" value="Сохранить">
        </form>
        <div class="info">Введите данные до трёх WiFi сетей. Подключение будет выполнено к первой доступной.</div>
    </div>
    <div class="footer">
        <p>© 2025 ESP32-CAM Camera Controller</p>
        <p>Version: %VERSION%</p>
    </div>
</body>
</html>
)rawliteral";
const char SETTINGS_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
    <title>Настройки %OTA_HOST%</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            max-width: 800px;
            margin: 20px auto;
            padding: 20px;
            background-color: #f5f5f5;
            font-size: 16px;
            line-height: 1.5;
            -webkit-text-size-adjust: 100%;
        }
        .main-title {
            text-align: center;
            color: #333;
            margin-bottom: 30px;
            font-size: 24px;
        }
        form {
            background-color: white;
            padding: 20px;
            border-radius: 8px;
            box-shadow: 0 2px 4px rgba(0,0,0,0.1);
        }
        h2 {
            color: #333;
            border-bottom: 2px solid #eee;
            padding-bottom: 10px;
            margin-top: 20px;
            font-size: 20px;
        }
        label {
            display: flex;
            align-items: center;
            margin: 15px 0;
            color: #555;
            font-size: 16px;
        }
        label span {
            flex: 0 0 33.33%;
            padding-right: 10px;
            font-size: 16px;
        }
        input[type="text"],
        input[type="number"],
        input[type="password"],
        select {
            flex: 1;
            padding: 12px;
            border: 1px solid #ddd;
            border-radius: 4px;
            box-sizing: border-box;
            background-color: #f9f9f9;
            font-size: 16px;
            height: 45px;
            outline: none;
            transition: border-color 0.3s ease;
        }
        input[type="text"]:focus,
        input[type="number"]:focus,
        input[type="password"]:focus,
        select:focus {
            border-color: #4a90e2;
            box-shadow: 0 0 0 2px rgba(74, 144, 226, 0.2);
        }
        input[type="text"] {
            border: 1px solid #e0e0e0;
            padding: 12px;
            margin: 4px 0;
        }
        input[type="checkbox"] {
            margin-right: 8px;
            width: 20px;
            height: 20px;
        }
        input[type="submit"] {
            background-color: #4a90e2;
            color: white;
            padding: 15px 20px;
            border: none;
            border-radius: 4px;
            cursor: pointer;
            font-size: 16px;
            margin-top: 20px;
            width: 100%;
            height: 50px;
        }
        input[type="submit"]:hover {
            background-color: #357abd;
        }
        .section {
            margin-bottom: 30px;
        }
        .mqtt-topics {
            background-color: #f8f9fa;
            border-left: 4px solid #4a90e2;
            padding: 20px;
            margin: 15px 0;
            font-size: 15px;
            color: #666;
        }
        .mqtt-topics h3 {
            margin-top: 0;
            color: #333;
            font-size: 18px;
            margin-bottom: 15px;
        }
        .mqtt-topics ul {
            margin: 10px 0;
            padding-left: 20px;
        }
        .mqtt-topics li {
            margin: 8px 0;
            line-height: 1.4;
        }
        hr {
            border: none;
            border-top: 1px solid #e0e0e0;
            margin: 20px 0;
        }
        /* Стили для вкладок */
        .tab-buttons {
            display: flex;
            margin-bottom: 20px;
            border-bottom: 2px solid #ddd;
        }
        .tab-button {
            padding: 10px 20px;
            background: none;
            border: none;
            cursor: pointer;
            font-size: 16px;
            color: #666;
            border-bottom: 2px solid transparent;
            margin-bottom: -2px;
        }
        .tab-button.active {
            color: #4a90e2;
            border-bottom: 2px solid #4a90e2;
        }
        .tab-content {
            display: none;
        }
        .tab-content.active {
            display: block;
        }
        .brightness-control {
            display: flex;
            align-items: center;
            gap: 5px;
            flex: 1;
        }
        .brightness-control input {
            flex: 1;
        }
        .brightness-btn {
            background-color: #4a90e2;
            color: white;
            border: none;
            border-radius: 4px;
            width: 40px;
            height: 45px;
            font-size: 20px;
            cursor: pointer;
            display: flex;
            align-items: center;
            justify-content: center;
            transition: background-color 0.3s ease;
        }
        .brightness-btn:hover {
            background-color: #357abd;
        }
        .brightness-btn:disabled {
            background-color: #ccc;
            cursor: not-allowed;
        }
        @media (max-width: 600px) {
            body {
                padding: 10px;
            }
            form {
                padding: 15px;
            }
            label {
                flex-direction: column;
                align-items: flex-start;
                margin: 10px 0;
            }
            label span {
                flex: none;
                width: 100%;
                margin-bottom: 5px;
                padding-right: 0;
            }
            input[type="text"],
            input[type="number"],
            select {
                width: 100%;
                margin: 0;
                box-sizing: border-box;
            }
            .mqtt-topics {
                padding: 15px;
                font-size: 14px;
            }
            .mqtt-topics h3 {
                font-size: 16px;
            }
            .tab-buttons {
                flex-wrap: wrap;
            }
            .tab-button {
                flex: 1 1 50%;
                text-align: center;
            }
        }
        .lcd-info-box {
            background: #f8f9fa;
            border-left: 4px solid #4a90e2;
            padding: 20px;
            margin: 15px 0;
            font-size: 16px;
            color: #333;
            border-radius: 6px;
            box-shadow: 0 1px 3px rgba(0,0,0,0.04);
        }
        .lcd-info-box div {
            margin: 8px 0;
            display: flex;
            align-items: center;
        }
        .lcd-info-box strong {
            min-width: 110px;
            color: #4a90e2;
        }
        .footer {
            text-align: center;
            margin-top: 30px;
            color: #666;
            font-size: 14px;
        }
    </style>
</head>
<body>
    <h1 class="main-title">Настройки %OTA_HOST%</h1>
    
    <div class="tab-buttons">
        <button class="tab-button active" onclick="openTab('camera')">Камера</button>
        <button class="tab-button" onclick="openTab('mqtt')">MQTT</button>
        <button class="tab-button" onclick="openTab('led')">LED</button>
        <button class="tab-button" onclick="openTab('wifi')">WiFi</button>
        <button class="tab-button" onclick="openTab('lcd')">LCD</button>
        <button class="tab-button" onclick="openTab('serial-log')">Serial Log</button>
    </div>

    <form method='POST' action='/save'>
        <!-- Вкладка Камера -->
        <div id="camera" class="tab-content active">
            <div class="section">
                <h2>Настройки камеры</h2>
                <label><span>Flip Vertically:</span> <input type='checkbox' name='flip_vertical' %FLIP_VERTICAL%></label>
                <label><span>Resolution:</span> <select name='resolution'>
                    <option value='%FRAMESIZE_QQVGA%' %QQVGA_SELECTED%>160x120 (%FRAMESIZE_QQVGA%)</option>
                    <option value='%FRAMESIZE_QCIF%' %QCIF_SELECTED%>176x144 (%FRAMESIZE_QCIF%)</option>
                    <option value='%FRAMESIZE_HQVGA%' %HQVGA_SELECTED%>240x176 (%FRAMESIZE_HQVGA%)</option>
                    <option value='%FRAMESIZE_QVGA%' %QVGA_SELECTED%>320x240 (%FRAMESIZE_QVGA%)</option>
                    <option value='%FRAMESIZE_VGA%' %VGA_SELECTED%>640x480 (%FRAMESIZE_VGA%)</option>
                    <option value='%FRAMESIZE_SVGA%' %SVGA_SELECTED%>800x600 (%FRAMESIZE_SVGA%)</option>
                    <option value='%FRAMESIZE_XGA%' %XGA_SELECTED%>1024x768 (%FRAMESIZE_XGA%)</option>
                    <option value='%FRAMESIZE_HD%' %HD_SELECTED%>1280x720 (%FRAMESIZE_HD%)</option>
                    <option value='%FRAMESIZE_SXGA%' %SXGA_SELECTED%>1280x1024 (%FRAMESIZE_SXGA%)</option>
                    <option value='%FRAMESIZE_UXGA%' %UXGA_SELECTED%>1600x1200 (%FRAMESIZE_UXGA%)</option>
                </select></label>
                <label><span>Quality:</span> <select name='camera_quality'>
                    <option value='%QUALITY_5%' %QUALITY_5_SELECTED%>Best (%QUALITY_5%)</option>
                    <option value='%QUALITY_7%' %QUALITY_7_SELECTED%>Very High (%QUALITY_7%)</option>
                    <option value='%QUALITY_10%' %QUALITY_10_SELECTED%>High (%QUALITY_10%)</option>
                    <option value='%QUALITY_12%' %QUALITY_12_SELECTED%>Good (%QUALITY_12%)</option>
                    <option value='%QUALITY_15%' %QUALITY_15_SELECTED%>Standard (%QUALITY_15%)</option>
                    <option value='%QUALITY_17%' %QUALITY_17_SELECTED%>Balanced (%QUALITY_17%)</option>
                    <option value='%QUALITY_20%' %QUALITY_20_SELECTED%>Medium (%QUALITY_20%)</option>
                    <option value='%QUALITY_25%' %QUALITY_25_SELECTED%>Low (%QUALITY_25%)</option>
                    <option value='%QUALITY_30%' %QUALITY_30_SELECTED%>Very Low (%QUALITY_30%)</option>
                    <option value='%QUALITY_40%' %QUALITY_40_SELECTED%>Worst (%QUALITY_40%)</option>
                </select></label>
                <label><span>Яркость (-2;+2):</span> 
                    <div class="brightness-control">
                        <input type="text" name='brightness' id="brightness" value='%BRIGHTNESS%' readonly>
                        <button type="button" class="brightness-btn" onclick="adjustBrightness(-0.5)">-</button>
                        <button type="button" class="brightness-btn" onclick="adjustBrightness(0.5)">+</button>
                    </div>
                </label>
                
            </div>
        </div>

        <!-- Вкладка MQTT -->
        <div id="mqtt" class="tab-content">
            <div class="section">
                <h2>Настройки MQTT</h2>
                <label><span>Server:</span> <input type="text" name='mqtt_server' value='%MQTT_SERVER%'></label>
                <label><span>Port:</span> <input type="text" name='mqtt_port' value='%MQTT_PORT%'></label>
                <label><span>User:</span> <input type="text" name='mqtt_user' value='%MQTT_USER%'></label>
                <label><span>Password:</span> <input type="text" name='mqtt_password' value='%MQTT_PASSWORD%'></label>
                <hr>
                <label><span>Разрешить использовать встроенный светодиод:</span> <input type='checkbox' name='use_builtin_led' %USE_BUILDIN_LED%></label>
                <hr>
                <div class="mqtt-topics">
                    <h3>Доступные MQTT топики:</h3>
                    <p><strong>Корневой элемент:</strong> %MQTT_USER%</p>
                    <p><em>Все топики начинаются с корневого элемента, например: %MQTT_USER%/led/set</em></p>
                    <ul>
                        <li><strong>Подсветка камеры:</strong>
                            <ul>
                                <li>Топик: %MQTT_USER%/led/set
                                    <ul>
                                        <li>Сообщение: "on" - включить подсветку</li>
                                        <li>Сообщение: "off" - выключить подсветку</li>
                                        <li>Сообщение: "brightness [значение]" - установить яркость</li>
                                        <li>Сообщение: "rgb [r,g,b]" - установить цвет</li>
                                    </ul>
                                </li>
                                <li>Топик: %MQTT_USER%/led/state<br>Сообщение: {"state": "ON/OFF", "brightness": 0, "color": {"r": 255, "g": 255, "b": 255}}</li>
                            </ul>
                        </li>
                        <li><strong>Встроенный светодиод:</strong>
                            <ul>
                                <li>Топик: %MQTT_USER%/led_builtin/set
                                    <ul>
                                        <li>Сообщение: {"state": "ON"} - включить</li>
                                        <li>Сообщение: {"state": "OFF"} - выключить</li>
                                    </ul>
                                </li>
                                <li>Топик: %MQTT_USER%/led_builtin/state<br>Сообщение: {"state": "ON/OFF"}</li>
                            </ul>
                        </li>
                        <li><strong>Системные команды:</strong>
                            <ul>
                                <li>Топик: %MQTT_USER%/cmd<br>Сообщение: "restart" - перезапуск устройства</li>
                            </ul>
                        </li>
                        <li><strong>Управление камерой:</strong>
                            <ul>
                                <li>Топик: %MQTT_USER%/camera/set
                                    <ul>
                                        <li>Сообщение: {"resolution": [значение]} - установить разрешение (5-40)</li>
                                        <li>Сообщение: {"quality": [значение]} - установить качество (5-40)</li>
                                    </ul>
                                </li>
                                <li>Топик: %MQTT_USER%/camera/state<br>Сообщение: {"resolution": 10, "quality": 12}</li>
                            </ul>
                        </li>
                        <li><strong>Датчики:</strong>
                            <ul>
                                <li>Топик: %MQTT_USER%/temperature<br>Сообщение: числовое значение температуры</li>
                            </ul>
                        </li>
                    </ul>
                </div>
            </div>
        </div>

        <!-- Вкладка LED -->
        <div id="led" class="tab-content">
            <div class="section">
                <h2>Настройки подсветки</h2>
                <label><span>R:</span> <input type="text" name='led_r' value='%LED_R%'></label>
                <label><span>G:</span> <input type="text" name='led_g' value='%LED_G%'></label>
                <label><span>B:</span> <input type="text" name='led_b' value='%LED_B%'></label>
            </div>
        </div>

        <!-- Вкладка WiFi -->
        <div id="wifi" class="tab-content">
            <div class="section">
                <h2>Настройки WiFi</h2>
                <label><span>SSID 1:</span> <input type="text" name='ssid1' value='%WIFI_SSID1%'></label>
                <label><span>Пароль 1:</span> <input type="text" name='password1' value='%WIFI_PASSWORD1%'></label>
                <hr>
                <label><span>SSID 2:</span> <input type="text" name='ssid2' value='%WIFI_SSID2%'></label>
                <label><span>Пароль 2:</span> <input type="text" name='password2' value='%WIFI_PASSWORD2%'></label>
                <hr>
                <label><span>SSID 3:</span> <input type="text" name='ssid3' value='%WIFI_SSID3%'></label>
                <label><span>Пароль 3:</span> <input type="text" name='password3' value='%WIFI_PASSWORD3%'></label>
                <div class="mqtt-topics">
                    <h3>Информация:</h3>
                    <p>После изменения настроек WiFi устройство перезагрузится и попытается подключиться к одной из указанных сетей (по порядку).</p>
                    <p>Если подключение не удастся, устройство создаст точку доступа с именем "ESP32_CAM" и паролем "987654321S".</p>
                </div>
            </div>
        </div>

        <!-- Вкладка LCD -->
        <div id="lcd" class="tab-content">
            <div class="section">
                <h2>Информация LCD дисплея</h2>
                <div class="lcd-info-box">
                    <div><strong>WiFi:</strong> <span id="lcd-wifi">%LCD_WIFI%</span></div>
                    <div><strong>MQTT:</strong> <span id="lcd-mqtt">%LCD_MQTT%</span></div>
                    <div><strong>SSID:</strong> <span id="lcd-ssid">%LCD_SSID%</span></div>
                    <div><strong>IP:</strong> <span id="lcd-ip">%LCD_IP%</span></div>
                    <div><strong>OTA Host:</strong> <span id="lcd-ota">%LCD_OTA%</span></div>
                    <div><strong>Температура:</strong> <span id="lcd-temp">%LCD_TEMP%</span> &deg;C</div>
                </div>
            </div>
        </div>

        <!-- Вкладка Serial Log -->
        <div id="serial-log" class="tab-content">
            <div class="section">
                <h2>Serial Log</h2>
                <div id="serial-log-content" style="background: #f8f9fa; border: 1px solid #ddd; padding: 10px; height: 300px; overflow-y: auto; font-family: monospace; font-size: 14px; white-space: pre-wrap;">Загрузка...</div>
            </div>
        </div>

        <input type='submit' value='Сохранить'>
    </form>

    <div class="footer">
        <p>© 2024-2025 ESP32-CAM Camera Controller</p>
        <p>Version: %VERSION%</p>
    </div>

    <script>
        function openTab(tabName) {
            // Скрываем все вкладки
            var tabs = document.getElementsByClassName("tab-content");
            for (var i = 0; i < tabs.length; i++) {
                tabs[i].classList.remove("active");
            }
            
            // Убираем активный класс у всех кнопок
            var buttons = document.getElementsByClassName("tab-button");
            for (var i = 0; i < buttons.length; i++) {
                buttons[i].classList.remove("active");
            }
            
            // Показываем выбранную вкладку
            document.getElementById(tabName).classList.add("active");
            event.currentTarget.classList.add("active");
        }

        function adjustBrightness(delta) {
            const input = document.getElementById('brightness');
            let currentValue = parseFloat(input.value) || 0;
            let newValue = currentValue + delta;
            
            // Ограничиваем значение в диапазоне от -2 до +2
            if (newValue > 2) newValue = 2;
            if (newValue < -2) newValue = -2;
            
            // Округляем до одного знака после запятой
            newValue = Math.round(newValue * 10) / 10;
            
            input.value = newValue;
            
            // Обновляем состояние кнопок
            updateBrightnessButtons(newValue);
        }

        function updateBrightnessButtons(value) {
            const minusBtn = document.querySelector('.brightness-btn:first-of-type');
            const plusBtn = document.querySelector('.brightness-btn:last-of-type');
            
            minusBtn.disabled = value <= -2;
            plusBtn.disabled = value >= 2;
        }

        // Инициализация состояния кнопок при загрузке страницы
        document.addEventListener('DOMContentLoaded', function() {
            const brightnessInput = document.getElementById('brightness');
            updateBrightnessButtons(parseFloat(brightnessInput.value) || 0);
        });

        function fetchSerialLog() { 
            const logContent = document.getElementById('serial-log-content');
            const isScrolledToBottom = logContent.scrollHeight - logContent.scrollTop === logContent.clientHeight;

            fetch('/serial-log')
                .then(response => response.text())
                .then(data => {
                    logContent.textContent += data; // Добавляем новые строки к существующим
                    if (isScrolledToBottom) {
                        logContent.scrollTop = logContent.scrollHeight; // Автопрокрутка вниз, если пользователь не прокручивал
                    }
                })
                .catch(error => console.error('Error fetching serial log:', error));
        }
        
        setInterval(fetchSerialLog, 1000);
    </script>
</body>
</html>
)rawliteral";
