from flask import Flask, render_template, request, jsonify
import sqlite3
import time
import requests

app = Flask(__name__)

# Biến toàn cục để lưu trữ địa chỉ IP của ESP32
esp32_ip = None  # Khởi tạo là None

# Biến toàn cục để lưu trữ trạng thái nút nhấn (khởi tạo là 0)

# Khởi tạo database
conn = sqlite3.connect('data.db')
cursor = conn.cursor()
cursor.execute('''
    CREATE TABLE IF NOT EXISTS history (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        voltage REAL,
        current REAL,
        power REAL,
        ledState INTEGER, 
        timestamp TEXT
    )
''')
conn.commit()
conn.close()

def get_db_connection():
    conn = sqlite3.connect('data.db')
    conn.row_factory = sqlite3.Row
    return conn

def get_last_led_state():
    conn = get_db_connection()
    cursor = conn.cursor()
    try:
        cursor.execute(
            'SELECT ledState FROM history ORDER BY id DESC LIMIT 1')
        result = cursor.fetchone()
        conn.close()
        if result:
            return result[0]
        else:
            return 0  # Trạng thái mặc định nếu chưa có dữ liệu
    except Exception as e:
        print(f"Error getting last LED state: {e}")
        return 0

led_state = get_last_led_state()  

@app.route('/')
def index():
    return render_template('index.html')

@app.route('/upload', methods=['GET'])
def upload():
    global esp32_ip  # Khai báo biến global
    voltage = request.args.get('voltage')
    current = request.args.get('current')
    power = request.args.get('power')
    ledState = request.args.get('ledState')  # Nhận ledState từ ESP32
    esp32_ip = request.args.get('esp32_ip')  # Nhận địa chỉ IP từ ESP32

    # Lưu dữ liệu vào database
    conn = get_db_connection()
    cursor = conn.cursor()
    timestamp = time.strftime('%Y-%m-%d %H:%M:%S')
    cursor.execute(
        'INSERT INTO history (voltage, current, power, ledState, timestamp) VALUES (?, ?, ?, ?, ?)',
        (voltage, current, power, ledState, timestamp))
    conn.commit()
    conn.close()

    # Trả về trạng thái nút nhấn
    global led_state
    return jsonify({'ledState': led_state})

@app.route('/history')
def history():
    conn = get_db_connection()
    cursor = conn.cursor()

    page = int(request.args.get('page', 1))  # Lấy số trang hiện tại
    per_page = 10  # Số dòng mỗi trang

    # Tính toán OFFSET cho câu truy vấn SQL
    offset = (page - 1) * per_page

    cursor.execute('SELECT COUNT(*) FROM history')
    total_rows = cursor.fetchone()[0]
    total_pages = (total_rows + per_page - 1) // per_page

    cursor.execute(
        'SELECT * FROM history ORDER BY id DESC LIMIT ? OFFSET ?',
        (per_page, offset))
    data = cursor.fetchall()
    conn.close()
    return render_template('history.html',
                           data=data,
                           current_page=page,
                           total_pages=total_pages)

@app.route('/led', methods=['GET'])
def led_control():
    global led_state
    state = request.args.get('state')
    if state == "on":
        led_state = 1
    elif state == "off":
        led_state = 0

    return "LED state updated"

@app.route('/toggle_led', methods=['POST'])
def toggle_led():
    global led_state
    led_state = 1 - led_state  # Đảo trạng thái led_state

    # Gửi request đến ESP32 để điều khiển đèn
    if esp32_ip:  # Kiểm tra nếu đã nhận được địa chỉ IP từ ESP32
        esp32_url = f"http://{esp32_ip}/control?state={'on' if led_state == 1 else 'off'}"
        try:
            response = requests.get(esp32_url)
            response.raise_for_status()  # Kiểm tra lỗi
            return "LED toggled successfully!"
        except requests.exceptions.RequestException as e:
            return f"Error toggling LED: {e}"
    else:
        return "ESP32 IP address not found!"

@app.route('/data')
def data():
    conn = get_db_connection()
    cursor = conn.cursor()
    cursor.execute(
        'SELECT voltage, current, power, ledState FROM history ORDER BY id DESC LIMIT 1'
    )
    data = cursor.fetchone()
    conn.close()

    global led_state
    if data:  # Check if data is not None
        return jsonify({'voltage': data[0], 'current': data[1],
                        'power': data[2], 'ledState': led_state})
    else:
        return jsonify({'voltage': 0, 'current': 0, 'power': 0,
                        'ledState':  led_state})  # Return default values

if __name__ == '__main__':
    app.run(debug=True, host='0.0.0.0')