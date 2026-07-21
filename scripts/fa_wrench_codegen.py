#!/usr/bin/env python3
"""Generate fixed OSQP workspaces for the PX4 FA control allocator.

This is an offline build tool. It defines the fixed six-rotor geometry and
constructs the same raw effectiveness matrix used by PX4's rotor effectiveness
model. The generated workspaces are compiled into ControlAllocationQP.
"""

# defines
FA_FEASIBILITY_TOL = 5.00e-1
FA_MATRIX_TOL = 1.00e-3


from pathlib import Path
import shutil

import numpy as np
import osqp
from scipy import sparse

# ==========================================================================
# BLOCK 1: FIXED ROTOR DATA AND PX4 EFFECTIVENESS MATRIX (FRD CONVERTED)
# ==========================================================================

deg = np.pi / 180.0

gamma = np.array([30.0, 90.0, 150.0, 210.0, 270.0, 330.0]) * deg # ccw about the up axis of the drone 
alpha = np.array([-25.0, 25.0, -25.0, 25.0, -25.0, 25.0]) * deg # rotation axis uses rhr reletive to OUTSIDE the drone's arms
beta = np.zeros(6) # zero beta tilts for now
spin = np.array([1.0, -1.0, 1.0, -1.0, 1.0, -1.0]) # +1 = ccw, -1 = cw
kappa = 0.0185 * np.ones(6) # torque to thrust ratio per rotor

horizontal_radius = 0.360 # arm radius
z_offset_flu = 0.0440 # m UP (positive in FLU, will become negative in FRD)

Np = gamma.size

position = np.zeros((3, Np))
axis = np.zeros((3, Np))
CT = np.ones(Np) * 17.658

mu_min = np.zeros(Np)
mu_max = CT * np.ones(Np) # now uses N & Nm

e1 = np.array([1.0, 0.0, 0.0])
e3 = np.array([0.0, 0.0, 1.0])

# compute intermediate raw positions and axes in FLU first
for i in range(Np):
    cg, sg = np.cos(gamma[i]), np.sin(gamma[i])
    ca, sa = np.cos(alpha[i]), np.sin(alpha[i])
    cb, sb = np.cos(beta[i]), np.sin(beta[i])

    Rz = np.array([
        [cg, -sg, 0.0],
        [sg,  cg, 0.0],
        [0.0, 0.0, 1.0],
    ])

    Rx_alpha = np.array([
        [1.0, 0.0, 0.0],
        [0.0, ca, -sa],
        [0.0, sa,  ca],
    ])

    Ry_beta = np.array([
        [cb, 0.0, sb],
        [0.0, 1.0, 0.0],
        [-sb, 0.0, cb],
    ])

    # calculate flat horizontal position, then apply pure Z offset
    position[:, i] = horizontal_radius * (Rz @ e1)
    position[2, i] = z_offset_flu

    # apply only structural roll (beta), pitch (alpha), and yaw (gamma)
    axis[:, i] = Rz @ Rx_alpha @ Ry_beta @ e3
    axis[:, i] /= np.linalg.norm(axis[:, i])

# FLU -> FRD transform
position[1, :] = -position[1, :]
position[2, :] = -position[2, :]

axis[1, :] = -axis[1, :]
axis[2, :] = -axis[2, :]

# positive KM for CCW spin, negative for CW spin.
KM = kappa * spin

# generate B natively
B = np.zeros((6, Np))
for i in range(Np):
    # PX4 Vector allocation formula: torque = (pos x thrust) - (KM * CT * axis)
    #thrust_i = CT[i] * axis[:, i]
    #moment_i = np.cross(position[:, i], thrust_i) - KM[i] * CT[i] * axis[:, i]
    
    # allocation matrix built agnostic of CT
    thrust_i = axis[:, i]
    moment_i = np.cross(position[:, i], thrust_i) - KM[i] * axis[:, i]

    B[0:3, i] = moment_i
    B[3:6, i] = thrust_i

if np.linalg.matrix_rank(B) != 6:
    raise RuntimeError("The configured PX4 effectiveness matrix does not have rank six.")

# ==========================================================================
# BLOCK 2: VECTOR & MATRIX CREATION
# ==========================================================================

sw = np.array([200.0, 200.0, 100.0, 1.0, 1.0, 3.0])
Sw = sparse.diags(sw, format="csc")
Aq = Sw @ sparse.csc_matrix(B)

mu_initial = 0.5 * mu_max # typical operation is 0.5 mu max (e.g. hover)
control_initial = B @ mu_initial
b_initial = sw * control_initial

# ==========================================================================
# BLOCK 3: CODE-GENERATION OUTPUT
# ==========================================================================

script_dir = Path(__file__).resolve().parent

repo_root = script_dir.parent

generated = (repo_root / "px4/PX4-Autopilot/src/lib/control_allocation/control_allocation/generated").resolve()

if generated.exists():
    shutil.rmtree(generated)

generated.mkdir(parents=False)

codegen_settings = dict(
    parameters="vectors",
    extension_name=None,
    use_float=True,
    printing_enable=False,
    profiling_enable=False,
    interrupt_enable=False,
    compile=False,
)

# ==========================================================================
# BLOCK 4: CLOSEST ACHIEVABLE CONTROL WORKSPACE
# ==========================================================================

P_closest_dense = (Aq.T @ Aq).toarray()
P_closest = sparse.csc_matrix(np.triu(P_closest_dense))
q_closest = -(Aq.T @ b_initial)

A_closest = sparse.eye(Np, format="csc")
l_closest = mu_min.copy()
u_closest = mu_max.copy()

closest_solver = osqp.OSQP()
closest_solver.setup(
    P=P_closest,
    q=np.asarray(q_closest).reshape(-1),
    A=A_closest,
    l=l_closest,
    u=u_closest,
    verbose=True,
    warm_starting=True,
    polishing=False,
    eps_abs=1e-3,
    eps_rel=1e-3,
    max_iter=4000,
    rho=0.5,
    adaptive_rho=False,
)

closest_solver.solve() # shows diagnostics, use adaptive rho to compute optimal values

closest_solver.codegen(
    generated,
    force_rewrite=True,
    include_codegen_src=True,
    prefix="fa_closest_",
    **codegen_settings,
)

# ==========================================================================
# BLOCK 5: MAXIMUM SATURATION-MARGIN WORKSPACE
# ==========================================================================

actuator_range = mu_max - mu_min
n_margin = Np + 1

P_margin = sparse.csc_matrix((n_margin, n_margin))
q_margin = np.zeros(n_margin)
q_margin[-1] = -1.0

A_margin_eq = sparse.hstack(
    [Aq, sparse.csc_matrix((6, 1))],
    format="csc",
)
A_margin_lower = sparse.hstack(
    [-sparse.eye(Np, format="csc"), sparse.csc_matrix(actuator_range.reshape(-1, 1))],
    format="csc",
)
A_margin_upper = sparse.hstack(
    [sparse.eye(Np, format="csc"), sparse.csc_matrix(actuator_range.reshape(-1, 1))],
    format="csc",
)
A_margin_m = sparse.csc_matrix((np.ones(1), ([0], [Np])), shape=(1, n_margin))

A_margin = sparse.vstack(
    [A_margin_eq, A_margin_lower, A_margin_upper, A_margin_m],
    format="csc",
)

l_margin = np.concatenate([
    b_initial,
    -np.inf * np.ones(Np),
    -np.inf * np.ones(Np),
    np.array([0.0]),
])

u_margin = np.concatenate([
    b_initial,
    -mu_min,
    mu_max,
    np.array([0.5]),
])

margin_solver = osqp.OSQP()
margin_solver.setup(
    P=P_margin,
    q=q_margin,
    A=A_margin,
    l=l_margin,
    u=u_margin,
    verbose=True,
    warm_starting=True,
    polishing=False,
    eps_abs=1e-3,
    eps_rel=1e-3,
    max_iter=4000,
    rho=1.00e-06,
    adaptive_rho=False,
)

margin_solver.solve()

margin_solver.codegen(
    generated,
    force_rewrite=False,
    include_codegen_src=False,
    prefix="fa_margin_",
    **codegen_settings,
)

# ==========================================================================
# BLOCK 6: FIXED DATA SHARED BY THE PX4 ALLOCATOR AND GENERATED QPs
# ==========================================================================

problem_header = generated / "fa_problem_data.h"

with problem_header.open("w", encoding="utf-8") as f:
    f.write("/* Generated by fa_wrench_codegen.py.*/\n")
    f.write("#ifndef FA_PROBLEM_DATA_H\n#define FA_PROBLEM_DATA_H\n\n")
    f.write('#include "osqp.h"\n\n')
    f.write("#define FA_WRENCH_DIM 6\n")
    f.write(f"#define FA_NP {Np}\n")
    f.write(f"#define FA_MARGIN_CONSTRAINTS {6 + 2 * Np + 1}\n")
    f.write(f"#define FA_FEASIBILITY_TOL ((OSQPFloat){FA_FEASIBILITY_TOL:.2e})\n")
    f.write(f"#define FA_MATRIX_TOL ((OSQPFloat){FA_MATRIX_TOL:.2e})\n\n")

    f.write("static const OSQPFloat FA_B[FA_WRENCH_DIM][FA_NP] = {\n")
    for row in B:
        values = ", ".join(f"(OSQPFloat){x:.17g}" for x in row)
        f.write(f"    {{{values}}},\n")
    f.write("};\n\n")

    values = ", ".join(f"(OSQPFloat){x:.17g}" for x in sw)
    f.write(f"static const OSQPFloat FA_SW[FA_WRENCH_DIM] = {{{values}}};\n")

    values = ", ".join(f"(OSQPFloat){x:.17g}" for x in mu_min)
    f.write(f"static const OSQPFloat FA_MU_MIN[FA_NP] = {{{values}}};\n")

    values = ", ".join(f"(OSQPFloat){x:.17g}" for x in mu_max)
    f.write(f"static const OSQPFloat FA_MU_MAX[FA_NP] = {{{values}}};\n\n")

    f.write("#endif /* FA_PROBLEM_DATA_H */\n")

# Also emit the PX4 parameter commands corresponding to the same fixed data.
parameter_commands = generated / "fa_px4_param_commands.txt"
with parameter_commands.open("w", encoding="utf-8") as f:
    f.write("# Generated from the same fixed data as FA_B.\n")
    f.write("param set CA_AIRFRAME 16\n")
    f.write("param set CA_METHOD 3\n")
    f.write("param set CA_FAILURE_MODE 0\n")
    f.write("param set CA_R_REV 0\n")
    f.write(f"param set CA_ROTOR_COUNT {Np}\n")

    for i in range(Np):
        f.write(f"param set CA_ROTOR{i}_PX {position[0, i]:.9g}\n")
        f.write(f"param set CA_ROTOR{i}_PY {position[1, i]:.9g}\n")
        f.write(f"param set CA_ROTOR{i}_PZ {position[2, i]:.9g}\n")
        f.write(f"param set CA_ROTOR{i}_AX {axis[0, i]:.9g}\n")
        f.write(f"param set CA_ROTOR{i}_AY {axis[1, i]:.9g}\n")
        f.write(f"param set CA_ROTOR{i}_AZ {axis[2, i]:.9g}\n")
        f.write(f"param set CA_ROTOR{i}_CT {CT[i]:.9g}\n")
        f.write(f"param set CA_ROTOR{i}_KM {KM[i]:.9g}\n")

print(f"OSQP version: {osqp.__version__}")
print(f"PX4 effectiveness-matrix rank: {np.linalg.matrix_rank(B)}")
print(f"Generated workspaces: {generated}")