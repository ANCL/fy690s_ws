import numpy as np
import math

def calculate_rotor_vectors(i, alpha_rad, beta_rad, gamma_rad, L, x_offset, y_offset, z_offset, h, theta0=0.0):
    """Calculates the position and thrust vectors for a single rotor in the PX4 FRD frame."""
    theta = theta0 + i * (math.pi / 3.0)
    phi_rad = theta + gamma_rad

    # Thrust in Gazebo FLU
    v_flu_x = -math.cos(phi_rad) * math.sin(beta_rad) * math.cos(alpha_rad) - math.sin(phi_rad) * math.sin(alpha_rad)
    v_flu_y = -math.sin(phi_rad) * math.sin(beta_rad) * math.cos(alpha_rad) + math.cos(phi_rad) * math.sin(alpha_rad)
    v_flu_z = math.cos(alpha_rad) * math.cos(beta_rad)

    # Position in Gazebo FLU
    # motor height acts in thrust direction
    x_flu = x_offset + L * math.cos(phi_rad) + h * v_flu_x
    y_flu = y_offset + L * math.sin(phi_rad) + h * v_flu_y
    z_flu = z_offset + h * v_flu_z

    # Convert to PX4 FRD
    px, py, pz = x_flu, -y_flu, -z_flu
    ax, ay, az = v_flu_x, -v_flu_y, -v_flu_z

    # Clean up floating point noise
    px = 0.0 if abs(px) < 1e-5 else px
    py = 0.0 if abs(py) < 1e-5 else py
    pz = 0.0 if abs(pz) < 1e-5 else pz
    ax = 0.0 if abs(ax) < 1e-5 else ax
    ay = 0.0 if abs(ay) < 1e-5 else ay
    az = 0.0 if abs(az) < 1e-5 else az

    r_i = np.array([px, py, pz])
    f_i = np.array([ax, ay, az])
    
    return r_i, f_i


def build_allocation_matrix(L, x_offset, y_offset, z_offset, h, alphas, betas, gammas, km_values, theta0=0.0):
    """Builds the 6x6 allocation matrix and returns it alongside the physical vectors."""
    B = np.zeros((6, 6))
    positions = []
    thrusts = []

    for i in range(6):
        r_i, f_i = calculate_rotor_vectors(i, alphas[i], betas[i], gammas[i], L, x_offset, y_offset, z_offset, h, theta0)
        positions.append(r_i)
        thrusts.append(f_i)
        
        force_col = f_i
        
        # Note: Ensure km_values[i] includes the correct sign for rotor spin direction (+/-)
        torque_col = np.cross(r_i, f_i) - km_values[i] * f_i
        B[:, i] = np.hstack((force_col, torque_col))

    return B, positions, thrusts


def print_matrix_info(B):
    print("B matrix:")
    print(np.round(B, 4))
    print("\nRank(B):", np.linalg.matrix_rank(B))
    print("Condition number cond(B):", round(np.linalg.cond(B), 4))