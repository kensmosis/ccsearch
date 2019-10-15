import numpy as np
import numpy.ctypeslib as ctl
import ctypes

# This loads the C++ library and declares the Python API fns
# See OAPI.h for details of the underlying functions
def ccsapi():
	# Load our external dll
	global libname
	libname = 'ccslib.so'
	global libdir
	libdir = './obj/'
	global cm
	cm= ctl.load_library(libname, libdir)

	# Define the various API fns
	global py_ccs_init_struct
	py_ccs_init_struct= cm.kopt_init_struct
	py_ccs_init_struct.argtypes = [ctypes.c_int, ctypes.c_int, ctl.ndpointer(np.int32, flags='aligned, c_contiguous'), ctypes.c_int, ctypes.c_int, ctypes.c_float, ctypes.c_int]

	global py_ccs_init_parms
	py_ccs_init_parms= cm.kopt_init_parms
	py_ccs_init_parms.argtypes = [ctypes.c_float,  ctypes.c_float, ctypes.c_int, ctypes.c_int, ctypes.c_long, ctypes.c_int]

	global py_ccs_init_feature
	py_ccs_init_feature= cm.kopt_init_feature
	py_ccs_init_feature.argtypes = [ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctl.ndpointer(np.intp, flags='aligned, c_contiguous')]

	global py_ccs_init_items
	py_ccs_init_items= cm.kopt_init_items
	py_ccs_init_items.argtypes = [ctl.ndpointer(np.float32, flags='aligned,  c_contiguous'),ctl.ndpointer(np.float32, flags='aligned, c_contiguous')]

	global py_ccs_set_constraint
	py_ccs_set_constraint= cm.kopt_set_constraint
	py_ccs_set_constraint.argtypes = [ctypes.c_int, ctypes.c_int, ctypes.c_int,  ctl.ndpointer(np.int32, flags='aligned, c_contiguous')]

	global py_ccs_set_maxcosttol
	py_ccs_set_maxcosttol= cm.kopt_set_maxcosttol
	py_ccs_set_maxcosttol.argtypes = [ctypes.c_float]
	
	global py_ccs_lock_and_load
	py_ccs_lock_and_load= cm.kopt_lock_and_load
	py_ccs_lock_and_load.restype= ctypes.c_int

	global py_ccs_get_log_state_space_est
	py_ccs_get_log_state_space_est= cm.kopt_get_log_state_space_est
	py_ccs_get_log_state_space_est.restype= ctypes.c_double

	global py_ccs_execute
	py_ccs_execute= cm.kopt_execute
	py_ccs_execute.restype= ctypes.c_int
	py_ccs_execute.argtypes = [ctypes.c_int]

	global py_ccs_prepres
	py_ccs_prepres= cm.kopt_prepres
	py_ccs_prepres.restype= ctypes.c_int

	global py_ccs_colllen
	py_ccs_colllen= cm.kopt_colllen
	py_ccs_colllen.restype= ctypes.c_int

	global py_ccs_getres
	py_ccs_getres= cm.kopt_getres
	py_ccs_getres.argtypes = [ctypes.c_int, ctl.ndpointer(np.intp, flags='aligned, c_contiguous'),ctl.ndpointer(np.float32, flags='aligned, c_contiguous')]
	py_ccs_getres.restype= ctypes.c_int

	global py_ccs_release
	py_ccs_release= cm.kopt_release

	global py_ccs_reset
	py_ccs_release= cm.kopt_reset

