import numpy as np
import math

def calculate_rotor_vectors(i, alpha_rad, beta_rad, gamma_rad, L, km, theta0=0.0):
    """Calculates the position and thrust vectors for a single rotor in the PX4 FRD frame."""
    theta = theta0 + i * (math.pi / 3.0)
    phi_rad = theta + gamma_rad
    
    # 1. Position in Gazebo FLU
    x_flu = L * math.cos(phi_rad)
    y_flu = L * math.sin(phi_rad)
    
    # 2. Thrust in Gazebo FLU
    v_flu_x = -math.cos(phi_rad) * math.sin(beta_rad) * math.cos(alpha_rad) - math.sin(phi_rad) * math.sin(alpha_rad)
    v_flu_y = -math.sin(phi_rad) * math.sin(beta_rad) * math.cos(alpha_rad) + math.cos(phi_rad) * math.sin(alpha_rad)
    v_flu_z = math.cos(alpha_rad) * math.cos(beta_rad)

    # 3. Convert to PX4 FRD
    px, py, pz = x_flu, -y_flu, -0.0440
    ax, ay, az = v_flu_x, -v_flu_y, -v_flu_z

    # Clean up floating point noise
    px = 0.0 if abs(px) < 1e-5 else px
    py = 0.0 if abs(py) < 1e-5 else py
    ax = 0.0 if abs(ax) < 1e-5 else ax
    ay = 0.0 if abs(ay) < 1e-5 else ay
    az = 0.0 if abs(az) < 1e-5 else az

    r_i = np.array([px, py, pz])
    f_i = np.array([ax, ay, az])
    
    return r_i, f_i


def build_allocation_matrix(L, alphas, betas, gammas, km_values, theta0=0.0):
    """Builds the 6x6 allocation matrix and returns it alongside the physical vectors."""
    B = np.zeros((6, 6))
    positions = []
    thrusts = []

    for i in range(6):
        r_i, f_i = calculate_rotor_vectors(i, alphas[i], betas[i], gammas[i], L, km_values[i], theta0)
        positions.append(r_i)
        thrusts.append(f_i)
        
        force_col = f_i
        torque_col = np.cross(r_i, f_i) + km_values[i] * f_i
        B[:, i] = np.hstack((force_col, torque_col))

    return B, positions, thrusts


def print_matrix_info(B):
    print("B matrix:")
    print(np.round(B, 4))
    print("\nRank(B):", np.linalg.matrix_rank(B))
    print("Condition number cond(B):", round(np.linalg.cond(B), 4))


if __name__ == "__main__":
    # Test values if the script is run completely by itself
    L = 0.360
    km = np.array([0.0185, -0.0185, 0.0185, -0.0185, 0.0185, -0.0185])
    
    # Example: 25 deg alpha alternating, 0 deg beta
    test_alphas = [math.radians(x) for x in [25.0, -25.0, 25.0, -25.0, 25.0, -25.0]]
    test_betas = [0.0] * 6
    test_gammas = [0.0] * 6

    B, pos, thrust = build_allocation_matrix(L, test_alphas, test_betas, test_gammas, km, theta0=math.radians(30.0))
    print_matrix_info(B)