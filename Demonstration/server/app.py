from flask import Flask, render_template, jsonify
import serial
import threading
from datetime import datetime
import pandas as pd

app = Flask(__name__)

# Dictionnaire pour stocker les données du capteur
sensor_data = {
    'lat': [46.2276],  # Position fixe
    'lon': [2.2137],  # Position fixe
    'date': [],
    'temperature': [],
    'humidity': [],
    'fire_risk': []
}

# Event pour stopper le thread proprement
stop_event = threading.Event()

def read_serial_data(stop_event):
    buffer = ""
    try:
        if ser.in_waiting > 0:
            ser = serial.Serial('/dev/ttyUSB0', 9600, timeout=1)
            print("Connected to /dev/ttyUSB0")
            while not stop_event.is_set():
                try:
                    data = ser.read(ser.in_waiting or 1).decode('utf-8', errors='ignore')
                    if data:
                        buffer += data
                        while 'ENDATA' in buffer:
                            start = buffer.find('DATA')
                            end = buffer.find('ENDATA', start)
                            if start != -1 and end != -1:
                                line = buffer[start:end+6]
                                buffer = buffer[end+6:]
                                print(f"Received: {line}")
                                parts = line.split()
                                if parts[0] == 'DATA' and parts[-1] == 'ENDATA' and len(parts) == 5:
                                    try:
                                        temperature = float(parts[1])
                                        humidity = float(parts[2])
                                        fire_risk = float(parts[3])
                                        current_time = datetime.now()

                                        # Ajouter les nouvelles données au dictionnaire
                                        if len(sensor_data['date']) >= 10:
                                            sensor_data['date'].pop(0)
                                            sensor_data['temperature'].pop(0)
                                            sensor_data['humidity'].pop(0)
                                            sensor_data['fire_risk'].pop(0)

                                        sensor_data['date'].append(current_time)
                                        sensor_data['temperature'].append(temperature)
                                        sensor_data['humidity'].append(humidity)
                                        sensor_data['fire_risk'].append(fire_risk)

                                        # Afficher les données actuelles du dictionnaire
                                        #print(sensor_data)
                                    except ValueError:
                                        print(f"Invalid data format: {line}")
                                else:
                                    print(f"Invalid data frame: {line}")
                    ser.reset_input_buffer()
                except serial.SerialException as e:
                    print(f"Error reading line from serial port: {e}")
                    stop_event.set()
                except Exception as e:
                    print(f"Unexpected error: {e}")
    except serial.SerialException as e:
        print(f"Serial exception: {e}")
        print("Failed to connect to /dev/ttyUSB0.")

@app.route('/')
def index():
    return render_template('map.html')

@app.route('/data')
def get_data():
    # Assurez-vous que sensor_data est bien structuré
    return jsonify({
        'lat': sensor_data['lat'][0],
        'lon': sensor_data['lon'][0],
        'date': sensor_data['date'],
        'temperature': sensor_data['temperature'],
        'humidity': sensor_data['humidity'],
        'fire_risk': sensor_data['fire_risk']
    })

@app.route('/sensor/<int:sensor_id>/data')
def sensor_data_endpoint(sensor_id):
    # Si vous avez plusieurs capteurs, gérez leur identifiant ici
    if sensor_id == 1:  # Par exemple, capteur avec ID 1
        return jsonify({
            'temperature': list(zip(sensor_data['date'], sensor_data['temperature'])),
            'humidity': list(zip(sensor_data['date'], sensor_data['humidity'])),
            'fire_risk': list(zip(sensor_data['date'], sensor_data['fire_risk']))
        })
    else:
        return jsonify({'error': 'Sensor not found'}), 404

# Créer et démarrer le thread de lecture série
serial_thread = threading.Thread(target=read_serial_data, args=(stop_event,), daemon=True)
serial_thread.start()

# Exemple de fonction pour arrêter proprement le thread et la lecture série
def stop_reading():
    stop_event.set()
    serial_thread.join()
    print("Stopped reading from /dev/ttyUSB0")

if __name__ == '__main__':
    try:
        app.run(debug=True)
    finally:
        stop_reading()
