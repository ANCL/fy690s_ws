import math
import sys

def generate_airframe_file(alphas_deg, betas_deg, gammas_deg, filename="6010_fa_hex_tilt"):
    """
    Generates a PX4 airframe configuration file with modified PX, PY, AX, AY, AZ.
    
    :param alphas_deg: List of 6 tilt arm axis angles in degrees.
    :param betas_deg: List of 6 tilt outward axis angles in degrees.
    :param gammas_deg: List of 6 placement offset angles in degrees.
    :param filename: Output filename.
    """
    
    # Base configuration constants
    base_angles = [30.0, 90.0, 150.0, 210.0, 270.0, 330.0]
    radius = 0.36  # Calculated from sqrt(0.3118^2 + 0.1800^2)
    pz_fixed = -0.0440
    
    # KM Values (CCW == -ve vs CW == +ve)
    km_vals = [0.0136, -0.0136, 0.0136, -0.0136, 0.0136, -0.0136]
    directions = ["CCW", "CW", "CCW", "CW", "CCW", "CW"]

    header = """#!/bin/sh

. ${R}etc/init.d/rc.mc_defaults

PX4_SIMULATOR=${PX4_SIMULATOR:=gz}
PX4_GZ_WORLD=${PX4_GZ_WORLD:=default}
PX4_SIM_MODEL=${PX4_SIM_MODEL:=fa_hex_tilt}

param set-default CA_ROTOR_COUNT 6
"""

    footer = """
# --- GAZEBO MOTOR MAPPINGS ---
param set-default SIM_GZ_EC_FUNC1 101
param set-default SIM_GZ_EC_FUNC2 102
param set-default SIM_GZ_EC_FUNC3 103
param set-default SIM_GZ_EC_FUNC4 104
param set-default SIM_GZ_EC_FUNC5 105
param set-default SIM_GZ_EC_FUNC6 106

param set-default SIM_GZ_EC_MIN1 150
param set-default SIM_GZ_EC_MIN2 150
param set-default SIM_GZ_EC_MIN3 150
param set-default SIM_GZ_EC_MIN4 150
param set-default SIM_GZ_EC_MIN5 150
param set-default SIM_GZ_EC_MIN6 150

param set-default SIM_GZ_EC_MAX1 1100
param set-default SIM_GZ_EC_MAX2 1100
param set-default SIM_GZ_EC_MAX3 1100
param set-default SIM_GZ_EC_MAX4 1100
param set-default SIM_GZ_EC_MAX5 1100
param set-default SIM_GZ_EC_MAX6 1100

param set-default SYS_AUTOSTART 6010
"""

    with open(filename, 'w') as f:
        f.write(header)

        for i in range(6):
            # Convert degrees to radians
            alpha = math.radians(alphas_deg[i])
            beta = math.radians(betas_deg[i])
            gamma = math.radians(gammas_deg[i])
            
            #  Calculate New Position based on Gamma
            phi_deg = base_angles[i] + gammas_deg[i]
            phi_rad = math.radians(phi_deg)
            
            px = radius * math.cos(phi_rad)
            py = -radius * math.sin(phi_rad)  # Y is negative for positive angles in this frame
            
            #  Calculate Thrust Vectors (AX, AY, AZ) using the rotation matrix
            # A_W = R_z(phi) * R_y(beta) * R_x(alpha) * [0, 0, -1]^T
            ax = -math.cos(phi_rad) * math.sin(beta) * math.cos(alpha) + math.sin(phi_rad) * math.sin(alpha)
            ay = math.sin(phi_rad) * math.sin(beta) * math.cos(alpha) + math.cos(phi_rad) * math.sin(alpha)
            az = -math.cos(beta) * math.cos(alpha)

            # Prevent floating point noise (e.g. 1.22e-16 to 0.0)
            px = 0.0 if abs(px) < 1e-5 else px
            py = 0.0 if abs(py) < 1e-5 else py
            ax = 0.0 if abs(ax) < 1e-5 else ax
            ay = 0.0 if abs(ay) < 1e-5 else ay
            az = 0.0 if abs(az) < 1e-5 else az

            f.write(f"\n# ==========================================\n")
            f.write(f"# ROTOR {i} (Base Angle: {base_angles[i]}°, {directions[i]})\n")
            f.write(f"# Offsets -> Alpha: {alphas_deg[i]}°, Beta: {betas_deg[i]}°, Gamma: {gammas_deg[i]}°\n")
            f.write(f"# ==========================================\n")
            f.write(f"param set-default CA_ROTOR{i}_PX {px:.4f}\n")
            f.write(f"param set-default CA_ROTOR{i}_PY {py:.4f}\n")
            f.write(f"param set-default CA_ROTOR{i}_PZ {pz_fixed:.4f}\n")
            f.write(f"param set-default CA_ROTOR{i}_AX {ax:.4f}\n")
            f.write(f"param set-default CA_ROTOR{i}_AY {ay:.4f}\n")
            f.write(f"param set-default CA_ROTOR{i}_AZ {az:.4f}\n")
            f.write(f"param set-default CA_ROTOR{i}_KM {km_vals[i]:.4f}\n")

        f.write(footer)
    
    print(f"Airframe configuration saved to '{filename}'.")

if __name__ == "__main__":
    # Example Usage:
    # Set alternating alphas (e.g., inwards/outwards) or fixed betas
    # If everything is set to 0.0, it will perfectly recreate your original file.
    
    test_alphas = [25.0, -25.0, 25.0, -25.0, 25.0, -25.0]
    test_betas  = [0.0, 0.0, 0.0, 0.0, 0.0, 0.0]
    test_gammas = [0.0, 0.0, 0.0, 0.0, 0.0, 0.0]
    
    # Example of an alternating inward/outward canted setup:
    # test_alphas = [15.0, -15.0, 15.0, -15.0, 15.0, -15.0]
    
    generate_airframe_file(test_alphas, test_betas, test_gammas)