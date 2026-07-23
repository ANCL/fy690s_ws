import math
import sys
import os
import numpy as np
from jinja2 import Template

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import check_fa_hex_allocation as checker

def generate_airframe_file(alphas_deg, betas_deg, gammas_deg, filename="6004_gz_fy690s_tilt"):
    radius = 0.360
    km_vals = [0.0185, -0.0185, 0.0185, -0.0185, 0.0185, -0.0185]
    ct_vals = [17.658, 17.658, 17.658, 17.658, 17.658, 17.658]
    directions = ["CCW", "CW", "CCW", "CW", "CCW", "CW"]
    base_angles = [30.0, 90.0, 150.0, 210.0, 270.0, 330.0]

    alphas_rad = [math.radians(a) for a in alphas_deg]
    betas_rad = [math.radians(b) for b in betas_deg]
    gammas_rad = [math.radians(g) for g in gammas_deg]

    # 1. Ask the checker script to compute the physics and build the matrix
    B_matrix, positions, thrusts = checker.build_allocation_matrix(
        L=radius, 
        alphas=alphas_rad, 
        betas=betas_rad, 
        gammas=gammas_rad, 
        km_values=km_vals, 
        theta0=math.radians(30.0)
    )

    # 2. Verify 6-DOF capability
    rank = np.linalg.matrix_rank(B_matrix)
    cond = np.linalg.cond(B_matrix)
    
    if rank < 6:
        raise ValueError(f"ABORT: Allocation matrix is rank-deficient (Rank {rank}/6). 6 DOFs cannot be achieved.")
    
    # Evaluate Condition Number (cond)
    if cond > 500:
        print(f"\n==================================================")
        print(f"CRITICAL ABORT: HIGHLY ILL-CONDITIONED MATRIX")
        print(f"Rank: {rank} | Condition Number: {cond:.2f}")
        print(f"SYSTEM APPROACHES SINGULARITY. FLIGHT PERFORMANCE")
        print(f"WILL BE UNACCEPTABLE. ABORTING GENERATION.")
        print(f"==================================================\n")
        raise ValueError(f"Matrix condition number ({cond:.2f}) exceeds maximum safe threshold of 500.")
    elif cond > 100:
        print(f"\n==================================================")
        print(f"WARNING: POORLY CONDITIONED MATRIX DETECTED")
        print(f"Rank: {rank} | Condition Number: {cond:.2f}")
        print(f"FLIGHT CONTROL AUTHORITY AND EFFICIENCY WILL BE REDUCED")
        print(f"==================================================\n")
    else:
        print(f"\n==================================================")
        print(f"STATUS: OPTIMAL 6-DOF CONFIGURATION VERIFIED")
        print(f"Rank: {rank} | Condition Number: {cond:.2f}")
        print(f"==================================================\n")

    # 3. Write Configuration using the vectors provided by the checker
    header = """#!/bin/sh
. ${R}etc/init.d/rc.fa_defaults
PX4_SIMULATOR=${PX4_SIMULATOR:=gz}
PX4_GZ_WORLD=${PX4_GZ_WORLD:=default}
PX4_SIM_MODEL=${PX4_SIM_MODEL:=fy690s_tilt}
"""
    footer = """
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
param set-default SIM_GZ_EC_MAX1 822.05
param set-default SIM_GZ_EC_MAX2 822.05
param set-default SIM_GZ_EC_MAX3 822.05
param set-default SIM_GZ_EC_MAX4 822.05
param set-default SIM_GZ_EC_MAX5 822.05
param set-default SIM_GZ_EC_MAX6 822.05
param set-default SYS_AUTOSTART 6004

# Wait 15 seconds for EKF to align, then command the mode switch to FA Position mode 
(sleep 15 && commander mode fa_position) &
"""

    with open(filename, 'w') as f:
        f.write(header)
        for i in range(6):
            px, py, pz = positions[i]
            ax, ay, az = thrusts[i]
            
            f.write(f"\n# ==========================================\n")
            f.write(f"# ROTOR {i} (Base Angle: {base_angles[i]}°, {directions[i]})\n")
            f.write(f"# Offsets -> Alpha: {alphas_deg[i]}°, Beta: {betas_deg[i]}°, Gamma: {gammas_deg[i]}°\n")
            f.write(f"# ==========================================\n")
            f.write(f"param set-default CA_ROTOR{i}_PX {px:.4f}\n")
            f.write(f"param set-default CA_ROTOR{i}_PY {py:.4f}\n")
            f.write(f"param set-default CA_ROTOR{i}_PZ {pz:.4f}\n")
            f.write(f"param set-default CA_ROTOR{i}_AX {ax:.4f}\n")
            f.write(f"param set-default CA_ROTOR{i}_AY {ay:.4f}\n")
            f.write(f"param set-default CA_ROTOR{i}_AZ {az:.4f}\n")
            f.write(f"param set-default CA_ROTOR{i}_KM {km_vals[i]:.4f}\n")
            f.write(f"param set-default CA_ROTOR{i}_CT {ct_vals[i]:.4f}\n")
        f.write(footer)
    print(f"Airframe configuration safely written to '{filename}'.")


def parse_input_vector(env_string, default_values, is_beta=False):
    if not env_string:
        return default_values
    if "," in env_string:
        try:
            parsed = [float(x.strip()) for x in env_string.split(",")]
            if len(parsed) != 6:
                print(f"ERROR: Array must contain exactly 6 elements. Got {len(parsed)}.")
                sys.exit(1)
            if is_beta:
                return [-abs(x) for x in parsed]
            return parsed
        except ValueError:
            print(f"ERROR: Could not parse array values from: {env_string}")
            sys.exit(1)
    else:
        try:
            mag = float(env_string)
            if is_beta:
                return [-abs(mag)] * 6
            else:
                return [mag, -mag, mag, -mag, mag, -mag]
        except ValueError:
            print(f"ERROR: Could not parse scalar value: {env_string}")
            sys.exit(1)


if __name__ == "__main__":
    repo_root = os.getcwd()
    airframe_out = os.path.join(repo_root, "ROMFS/px4fmu_common/init.d-posix/airframes/6004_gz_fy690s_tilt")
    jinja_template_path = os.path.join(repo_root, "Tools/simulation/gz/models/fy690s_tilt/fy690s_tilt.sdf.jinja")
    sdf_out_path = os.path.join(repo_root, "Tools/simulation/gz/models/fy690s_tilt/model.sdf")

    raw_alpha = os.environ.get("FA_ALPHA")
    raw_beta = os.environ.get("FA_BETA")

    test_alphas = parse_input_vector(raw_alpha, [25.0, -25.0, 25.0, -25.0, 25.0, -25.0])
    test_betas  = parse_input_vector(raw_beta, [0.0, 0.0, 0.0, 0.0, 0.0, 0.0], is_beta=True)
    test_gammas = [0.0, 0.0, 0.0, 0.0, 0.0, 0.0]

    print(f"\n--- Generating FY690S Airframe & SDF ---")
    print(f"Alphas mapped: {test_alphas}")
    print(f"Betas mapped:  {test_betas}")

    try:
        generate_airframe_file(test_alphas, test_betas, test_gammas, filename=airframe_out)
    except Exception as e:
        print(f"\n{e}")
        sys.exit(1)

    try:
        with open(jinja_template_path, "r") as f:
            template_content = f.read()

        template = Template(template_content)
        rendered_sdf = template.render(alphas=test_alphas, betas=test_betas)

        with open(sdf_out_path, "w") as f:
            f.write(rendered_sdf)

        print(f"Successfully compiled template into {sdf_out_path}")
        print("----------------------------------------\n")
    except Exception as e:
        print(f"ERROR rendering SDF: {e}")
        sys.exit(1)