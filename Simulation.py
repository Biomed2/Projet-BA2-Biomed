"""
Projet Biomed 2020-21
Groupe 2
Simulation du prototype de détection COVID-19
Paramètres : fichier .csv (Temps, Température, Taux d'oxygène, Battements de coeur par minute, Niveau de batterie)
1 mesure toutes les 5 minutes
Renvoie : fichier results.csv (Temps, Fièvre, Dyspnée, Tachycardie, Batterie faible, Température anormale,
Alerte envoyée par le dispositif)

TODO : Mesurer la consommation de l'ESP32 avec tous les capteurs et déterminer quand la batterie arrêtre de fonctionner
"""

import pandas as pd
import numpy as np

symptoms = pd.read_csv("data.csv")  # Opens data.csv file
symptoms["bat_low"] = np.where(symptoms["bat"] < 5, -1, 1)  # Adds a column "bat_low", True if bat < 5%

# Determine the first column (date-time) as index
symptoms.index = pd.to_datetime(symptoms.pop("Temps"))

# Create alerts dataframe
alerts = pd.DataFrame()

# Conditions for different fever levels
conditions = [
    (symptoms['fievre'] <= 37.5),
    (symptoms['fievre'] > 37.5) & (symptoms['fievre'] <= 38),
    (symptoms['fievre'] > 38) & (symptoms['fievre'] <= 39.5),
    (symptoms['fievre'] > 39.5)
]
# values = ['Normal', 'Bénin', 'Grave', 'Très Grave']
values = [1, 2, 3, 4]
symptoms['Fievre_niveau'] = np.select(conditions, values)

# To detect when no data has been received (ESP lost connection to Wifi)
symptoms["no_data_oxy"] = pd.isna(symptoms["oxy"])
symptoms["no_data_bpm"] = pd.isna(symptoms["bpm"])
symptoms["no_data"] = np.where((symptoms["no_data_bpm"] & symptoms["no_data_oxy"]), True, False)
symptoms = symptoms.drop(columns=["no_data_oxy", "no_data_bpm"])

# Check if no data has been received for the past 25 minutes
symptoms["t-1"] = symptoms['no_data'].shift(1)
symptoms["t-2"] = symptoms['no_data'].shift(2)
symptoms["t-3"] = symptoms['no_data'].shift(3)
symptoms["t-4"] = symptoms['no_data'].shift(4)
symptoms["t-5"] = symptoms['no_data'].shift(5)
symptoms["disconnected"] = np.where((symptoms['no_data'] & symptoms['t-1'] & symptoms['t-2'] & symptoms['t-3'] & symptoms['t-4'] & symptoms['t-5']), True, False)
symptoms = symptoms.drop(columns=["t-1", "t-2", "t-3", "t-4", "t-5"])

# Disconnect Alerts : send an alert when no data has been received for the last 25 minutes
alerts["disconnected"] = symptoms["disconnected"]

# Fever Alerts : send an alert when the fever level changes
symptoms["fever_diff"] = symptoms['Fievre_niveau'].diff()
symptoms["fever_level_alert"] = np.where(symptoms["fever_diff"] != float(0), symptoms["Fievre_niveau"], False)
conditions = [
    (symptoms['fever_level_alert'] == 1),
    (symptoms['fever_level_alert'] == 2),
    (symptoms['fever_level_alert'] == 3),
    (symptoms['fever_level_alert'] == 4),
]
values = ["Température normale", "Alerte : fièvre", "Alerte : fièvre grave", "Alerte : fièvre très grave"]
alerts['fever'] = np.select(conditions, values)

# Oxymeter alerts : send an alert when the oxygen concentration decreases to less than 95%
symptoms["oxy_low"] = np.where((symptoms["oxy"] < 95), 1, 2)

symptoms["dyspnea"] = symptoms["oxy_low"].diff()
conditions = [
    (symptoms['dyspnea'] == -1),
    (symptoms['dyspnea'] == 1)
]
values = ["Alerte : taux d'oxygène bas", "Taux d'oxygène normalisé"]
alerts['dyspnea'] = np.select(conditions, values)
print(symptoms.to_string())
print(alerts.to_string())

# BPM alerts : send an alert when the heartbeats per minute are higher than 100
symptoms["bpm_high"] = np.where((symptoms["bpm"] >= 100), 1, 2)

symptoms["tachycardia"] = symptoms["bpm_high"].diff()
conditions = [
    (symptoms["tachycardia"] == -1),
    (symptoms["tachycardia"] == 1)
]
values = ["Alerte : battements de coeur élevés", "Battements de coeur normalisés"]
alerts['tachycardia'] = np.select(conditions, values)

# Battery alert : send an alert when battery level is lower than 5%
alerts["battery_low"] = np.where(symptoms["bat_low"].diff() == -2, "Alerte : niveau de batterie faible", 0)
print(symptoms.to_string())
print(alerts.to_string())
