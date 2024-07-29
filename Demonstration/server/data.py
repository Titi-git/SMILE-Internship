import pandas as pd
from datetime import datetime, timedelta
import random

#initialisation d'une variable globale sensors vide
sensors = []

def generate_sensor_data():
    # Simuler des données pour 8 capteurs sur une période de 7 jours
    sensors = []
    for i in range(1, 9):
        lat = 45.0 + random.uniform(-0.05, 0.05)
        lon = 3.0 + random.uniform(-0.05, 0.05)
        for day in range(7):
            date = datetime.now() - timedelta(days=day)
            temperature = random.uniform(18, 24)
            humidity = random.uniform(50, 60)
            fire_risk = random.uniform(0, 30)
            
            # Déterminer la couleur du marqueur en fonction du fire_risk
            if fire_risk > 50:
                marker_color = 'red'
            else:
                marker_color = 'green'
                
            sensors.append([i, lat, lon, date, temperature, humidity, fire_risk, marker_color])
            
    #df = pd.DataFrame(sensors, columns=['sensor_id', 'lat', 'lon', 'date', 'temperature', 'humidity', 'fire_risk', 'marker_color'])
    return sensors

def get_sensors():
    global sensors
    #si variable globale sensors est null on la rempli
    if sensors == []:
        sensors = generate_sensor_data()
    df = pd.DataFrame(sensors, columns=['sensor_id', 'lat', 'lon', 'date', 'temperature', 'humidity', 'fire_risk', 'marker_color'])
    return df


def get_sensor_data(sensor_id):
    global sensors
    #si variable est null on la rempli
    if sensors == []:
        sensors = generate_sensor_data()
    df = pd.DataFrame(sensors, columns=['sensor_id', 'lat', 'lon', 'date', 'temperature', 'humidity', 'fire_risk', 'marker_color'])
    return df[df['sensor_id'] == sensor_id]
