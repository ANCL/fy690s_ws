import numpy as np

def skew_cross(a, b):
    return np.cross(a, b)

def rotor_axis(theta, alpha, tilt_direction="tangential"):
    if tilt_direction == "tangential":
        sign = 1.0
    else:
        sign = -1.0

    return np.array([
        sign * np.sin(alpha) * -np.sin(theta),
        sign * np.sin(alpha) * np.cos(theta),
        -np.cos(alpha)
    ])

def build_allocation_matrix(L, alpha, km_values, theta0=0.0, tilt_direction="outward"):
    B = np.zeros((6, 6))

    for i in range(6):
        theta = theta0 + i * np.pi / 3.0

        alpha_i = alpha if i % 2 == 0 else -alpha # Alternate rotor angle sign

        r_i = np.array([
            L * np.cos(theta),
            L * np.sin(theta),
            0.0
        ])

        a_i = rotor_axis(theta, alpha, tilt_direction)
        km_i = km_values[i]

        force_col = a_i
        torque_col = np.cross(r_i, a_i) + km_i * a_i

        B[:, i] = np.hstack((force_col, torque_col))

    return B

def print_matrix_info(B):
    print("B matrix:")
    print(B)

    print("\nRank(B):")
    print(np.linalg.matrix_rank(B))

    print("\nCondition number cond(B):")
    print(np.linalg.cond(B))

    print("\nPseudoinverse pinv(B):")
    print(np.linalg.pinv(B))

def test_single_axis_wrenches(B):
    test_names = ["Fx", "Fy", "Fz", "tau_x", "tau_y", "tau_z"]
    test_wrenches = np.eye(6)

    B_pinv = np.linalg.pinv(B)

    for name, w in zip(test_names, test_wrenches):
        f = B_pinv @ w
        w_reconstructed = B @ f

        print("\n==============================")
        print(f"Test wrench: {name}")
        print("Desired wrench:")
        print(w)
        print("Motor thrust solution:")
        print(f)
        print("Reconstructed wrench:")
        print(w_reconstructed)
        print("Minimum motor command:")
        print(np.min(f))
        print("Maximum motor command:")
        print(np.max(f))
        print("Reconstruction error norm:")
        print(np.linalg.norm(w - w_reconstructed))

if __name__ == "__main__":
    L = 0.345
    alpha_deg = 25.0
    alpha = np.deg2rad(alpha_deg)

    # Example alternating spin directions.
    # Adjust signs according to your real rotor order.
    km = np.array([0.0136, -0.0136, 0.0136, -0.0136, 0.0136, -0.0136])

    B = build_allocation_matrix(
        L=L,
        alpha=alpha,
        km_values=km,
        theta0=0.0,
        tilt_direction="outward"
    )

    print_matrix_info(B)
    test_single_axis_wrenches(B)