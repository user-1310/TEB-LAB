"""
Código de análisis y gráfica para el informe:
Caracterización de un sensor de presión manométrica con la Ley de Boyle.

Requisitos:
    pip install numpy matplotlib

Nota:
Los datos corresponden a los valores transcritos en las tablas del informe.
El ajuste lineal se presenta únicamente como análisis exploratorio: el R² es bajo
y existe deriva del cero, por lo que no debe considerarse una calibración válida.
"""

import numpy as np
import matplotlib.pyplot as plt

# ------------------------------------------------------------
# 1. Datos experimentales disponibles
# ------------------------------------------------------------
# Ciclo 1: compresión
vol_comp = np.array([1.00, 0.94, 0.88, 0.82, 0.76, 0.70, 0.64, 0.58])
pres_comp_kpa = np.array([0.000, 3.415, 7.069, 10.987, 15.199, 19.739, 24.647, 29.969])
cuentas_comp = np.array([
    4213656.0, 4209589.5, 4209414.5, 4207307.0,
    4207134.5, 4207228.0, 4206981.0, 4206622.0
])

# Ciclo 1: descompresión
vol_desc = np.array([0.64, 0.70, 0.76, 0.82, 0.88, 0.94, 1.00])
pres_desc_kpa = np.array([24.647, 19.739, 15.199, 10.987, 7.069, 3.415, 0.000])
cuentas_desc = np.array([
    4207989.5, 4208567.5, 4203706.5, 4206192.0,
    4205891.0, 4206370.5, 4206374.0
])

# Concatenar compresión y descompresión para el ajuste exploratorio
presion = np.concatenate([pres_comp_kpa, pres_desc_kpa])
cuentas = np.concatenate([cuentas_comp, cuentas_desc])

# ------------------------------------------------------------
# 2. Ajuste lineal: cuentas = sensibilidad * presión + intercepto
# ------------------------------------------------------------
sensibilidad, intercepto = np.polyfit(presion, cuentas, 1)
cuentas_estimadas = sensibilidad * presion + intercepto

ss_res = np.sum((cuentas - cuentas_estimadas) ** 2)
ss_tot = np.sum((cuentas - np.mean(cuentas)) ** 2)
r2 = 1 - ss_res / ss_tot if ss_tot != 0 else float("nan")

print("RESULTADOS DEL AJUSTE LINEAL EXPLORATORIO")
print(f"Sensibilidad (pendiente): {sensibilidad:.2f} cuentas/kPa")
print(f"Intercepto: {intercepto:.2f} cuentas")
print(f"Coeficiente de determinación R²: {r2:.4f}")
print("Advertencia: debido al bajo R² y a la deriva del cero,")
print("este ajuste no debe usarse como calibración válida.")

# ------------------------------------------------------------
# 3. Gráfica de la figura incluida en el informe
# ------------------------------------------------------------
presion_linea = np.linspace(float(np.min(presion)), float(np.max(presion)), 200)
cuentas_linea = sensibilidad * presion_linea + intercepto

plt.figure(figsize=(9, 6))
plt.scatter(pres_comp_kpa, cuentas_comp, label="Compresión (ciclo 1)", marker="o")
plt.scatter(pres_desc_kpa, cuentas_desc, label="Descompresión (ciclo 1)", marker="s")
plt.plot(
    presion_linea,
    cuentas_linea,
    label=f"Ajuste lineal exploratorio (R² = {r2:.3f})",
    linestyle="--"
)

plt.xlabel("Presión manométrica de referencia (kPa)")
plt.ylabel("Lectura media del sensor (cuentas)")
plt.title("Lectura del sensor frente a la presión de referencia")
plt.grid(True, alpha=0.3)
plt.legend()
plt.tight_layout()
plt.savefig("figura_ajuste_sensor_presion.png", dpi=300, bbox_inches="tight")
plt.show()

print("Figura guardada como: figura_ajuste_sensor_presion.png")
