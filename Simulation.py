"""
Projet Biomed 2020-21
Groupe 2
Simulation du prototype de détection COVID-19
Paramètres : fichier .csv (Temps, Température, Taux d'oxygène, Battements de coeur par minute, Niveau de batterie)
1 mesure toutes les 5 minutes
Renvoie : fichier results.csv (Temps, Fièvre, Dyspnée, Tachycardie, Batterie faible, Température anormale,
Alerte envoyée par le dispositif)

TODO : Mesurer la consommation de l'ESP32 avec tous les capteurs et déterminer quand la batterie arrêtre de fonctionner
TODO : inclure les alertes pour oxy et bpm, oxy : > 95% = 2, < 95% = 1, diff, -1 = taux low, +1 = taux normal
"""

import pandas as pd
import numpy as np

bat_low = False
fichier = True
temp_a = 0
temp_n = 1
oxy_n = 1
bpm_n = 1
last_5 = []
fievre = False
fievre_niv = 0  # temp < 37.5°C : 0, 37.5 < temp < 38.5 : 1, 38.5 < temp < 40 : 2, temp > 40 : 3
dyspnee = False
bpm_eleve = False
sympt = []
last_alert = []  # Last alert sent

"""
def niveau_fievre(temp):
    if temp < 37.5:
        n = 0
    elif 37.5 <= temp < 38.5:
        n = 1
    elif 38.5 <= temp < 40:
        n = 2
    elif temp > 40:
        n = 3
    return n


results = open("results.txt", "w")
with open('data.csv') as data:
    csv_reader = csv.reader(data, delimiter=',')
    for row in csv_reader:
        if float(row[4]) < 5:  # Battery level below 5%
            bat_low = True
        if float(row[1]) > 45 or float(row[1]) < 30:  # Abnormal temperature above 45°C or below 30°C
            temp_a = 1
        elif 35 < float(row[1]) < 37.5:  # Normal temperature between 35 and 37°C
            temp_n = 1
        elif float(row[1]) >= 37.5:  # Fever above 37.5°C
            temp_n = 0
        if float(row[2]) < 95:  # Oxygen concentration in blood under 95%
            oxy_n = 0
        if float(row[3]) >= 100:  # Heartbeats per minute above 100
            bpm_n = 0
        last_5.append([bat_low, temp_a, temp_n, oxy_n, bpm_n])
        if len(last_5) > 5:
            del last_5[0]
            if (last_5[0][1] + last_5[1][1] + last_5[2][1] + last_5[3][1] + last_5[4][1]) / 5 >= 0.8:
                fievre = True
            else:
                fievre = False
            if (last_5[0][3] + last_5[1][3] + last_5[2][3] + last_5[3][3] + last_5[4][3]) / 5 >= 0.8:
                dyspnee = True
            else:
                dyspnee = False
            if (last_5[0][4] + last_5[1][4] + last_5[2][4] + last_5[3][4] + last_5[4][4]) / 5 >= 0.8:
                bpm_eleve = True
            else:
                bpm_eleve = False
            sympt = [row[0], fievre, niveau_fievre(float(row[1])), dyspnee, bpm_eleve, bat_low, bool(temp_a)]
        print(sympt)
        results.write("".join([str(elem) for elem in sympt]))
"""

symptoms = pd.read_csv("data.csv")  # Opens data.csv file
symptoms["bat_low"] = np.where(symptoms["bat"] < 5, True, False)  # Adds a column "bat_low", True if bat < 5%

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
alerts["fever"] = np.where(symptoms["fever_diff"] != float(0), symptoms["Fievre_niveau"], False)

# Oxymeter alerts : send an alert when the oxygen concentration decreases to less than 95%

