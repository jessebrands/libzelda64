/*
 * module.c: Python module entrypoint
 * Copyright (C) 2026 Jesse Gerard Brands
 *
 * This file is part of libzelda64.
 *
 * libzelda64 is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * libzelda64 is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libzelda64. If not, see <https://www.gnu.org/licenses/>.
 */

#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <zelda64/zelda64.h>

PyMODINIT_FUNC PyInit_zelda64(void);

PyDoc_STRVAR(zelda64_rom_doc,
             "Rom(source)\n"
             "--\n"
             "\n"
             "A Nintendo 64 Zelda ROM.");

struct zelda64_rom_object {
    PyObject_HEAD

    PyObject* source;
};

static int
zelda64_rom_init(PyObject* self, PyObject* args, PyObject* kwds) {
    struct zelda64_rom_object* rom = (struct zelda64_rom_object*) self;
    static char* keywords[] = {"source", NULL};
    PyObject* source = NULL;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O:Rom", keywords, &source)) {
        return -1;
    }

    Py_INCREF(source);
    Py_XDECREF(rom->source);
    rom->source = source;

    return 0;
}

static void
zelda64_rom_dealloc(PyObject* self) {
    struct zelda64_rom_object* rom = (struct zelda64_rom_object*) self;
    PyTypeObject* type = Py_TYPE(self);

    Py_CLEAR(rom->source);

    freefunc free_instance = PyType_GetSlot(type, Py_tp_free);
    free_instance(self);

    // A heap type's instances hold a reference to the type itself.
    Py_DECREF(type);
}

static PyType_Slot zelda64_rom_slots[] = {
    {Py_tp_doc, (void*) zelda64_rom_doc},
    {Py_tp_init, (void*) zelda64_rom_init},
    {Py_tp_dealloc, (void*) zelda64_rom_dealloc},
    {0, NULL},
};

static PyType_Spec zelda64_rom_spec = {
    .name = "zelda64.Rom",
    .basicsize = (int) sizeof(struct zelda64_rom_object),
    .itemsize = 0,
    .flags = Py_TPFLAGS_DEFAULT,
    .slots = zelda64_rom_slots,
};

static PyMethodDef zelda64_methods[] = {
    {NULL, NULL, 0, NULL},
};

PyDoc_STRVAR(zelda64_module_doc,
             "zelda64 is a module for manipulating Nintendo 64 ROMs.\n");

static struct PyModuleDef zelda64_module = {
    PyModuleDef_HEAD_INIT,
    .m_name = "zelda64",
    .m_doc = zelda64_module_doc,
    .m_methods = zelda64_methods,
};

PyMODINIT_FUNC PyInit_zelda64(void) {
    PyObject* m = PyModule_Create(&zelda64_module);
    if (m == NULL) {
        return NULL;
    }

    if (PyModule_AddStringConstant(m, "__version__", zelda64_version_string()) < 0) {
        goto cleanup_module;
    }

    PyObject* zelda64_rom_type = PyType_FromSpec(&zelda64_rom_spec);
    if (zelda64_rom_type == NULL) {
        goto cleanup_module;
    }

    Py_INCREF(zelda64_rom_type);
    if (PyModule_AddObject(m, "Rom", zelda64_rom_type) < 0) {
        Py_DECREF(zelda64_rom_type);
        goto cleanup_rom_type;
    }

    return m;

cleanup_rom_type:
    Py_CLEAR(zelda64_rom_type);
cleanup_module:
    Py_DECREF(m);
    return NULL;
}
