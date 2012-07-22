//     Copyright 2012, Kay Hayen, mailto:kayhayen@gmx.de
//
//     Part of "Nuitka", an optimizing Python compiler that is compatible and
//     integrates with CPython, but also works on its own.
//
//     Licensed under the Apache License, Version 2.0 (the "License");
//     you may not use this file except in compliance with the License.
//     You may obtain a copy of the License at
//
//        http://www.apache.org/licenses/LICENSE-2.0
//
//     Unless required by applicable law or agreed to in writing, software
//     distributed under the License is distributed on an "AS IS" BASIS,
//     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//     See the License for the specific language governing permissions and
//     limitations under the License.
//

#include "nuitka/prelude.hpp"

extern PyObject *_python_str_plain_compile;
extern PyObject *_python_str_plain_strip;
extern PyObject *_python_str_plain_read;

static PythonBuiltin _python_builtin_compile( &_python_str_plain_compile );

PyObject *COMPILE_CODE( PyObject *source_code, PyObject *file_name, PyObject *mode, int flags )
{
    // May be a source, but also could already be a compiled object, in which case this
    // should just return it.
    if ( PyCode_Check( source_code ) )
    {
        return INCREASE_REFCOUNT( source_code );
    }

    // Workaround leading whitespace causing a trouble to compile builtin, but not eval builtin
    PyObject *source;

    if (
        (
#if PYTHON_VERSION < 300
            PyString_Check( source_code ) ||
#endif
            PyUnicode_Check( source_code )
        ) &&
        strcmp( Nuitka_String_AsString( mode ), "exec" ) != 0
       )
    {
        // TODO: There is an API to call a method, use it instead.
        source = LOOKUP_ATTRIBUTE( source_code, _python_str_plain_strip );
        source = PyObject_CallFunctionObjArgs( source, NULL );

        assert( source );
    }
#if PYTHON_VERSION < 300
    // TODO: What does Python3 do here.
    else if ( PyFile_Check( source_code ) && strcmp( Nuitka_String_AsString( mode ), "exec" ) == 0 )
    {
        // TODO: There is an API to call a method, use it instead.
        source = LOOKUP_ATTRIBUTE( source_code, _python_str_plain_read );
        source = PyObject_CallFunctionObjArgs( source, NULL );

        assert( source );
    }
#endif
    else
    {
        source = source_code;
    }

    PyObjectTemporary future_flags( PyInt_FromLong( flags ) );

    return _python_builtin_compile.call_args(
        MAKE_TUPLE5(
            source,
            file_name,
            mode,
            future_flags.asObject(), // flags
            Py_True                  // dont_inherit
        )
    );
}

extern PyObject *_python_str_plain_open;

static PythonBuiltin _python_builtin_open( &_python_str_plain_open );

PyObject *OPEN_FILE( PyObject *file_name, PyObject *mode, PyObject *buffering )
{
    if ( file_name == NULL )
    {
        return _python_builtin_open.call();

    }
    else if ( mode == NULL )
    {
        return _python_builtin_open.call1(
            file_name
        );

    }
    else if ( buffering == NULL )
    {
        return _python_builtin_open.call_args(
            MAKE_TUPLE2(
               file_name,
               mode
            )
        );
    }
    else
    {
        return _python_builtin_open.call_args(
            MAKE_TUPLE3(
                file_name,
                mode,
                buffering
            )
        );
    }
}

PyObject *BUILTIN_CHR( unsigned char c )
{
    // TODO: A switch statement might be faster, because no object needs to be created at
    // all, this is how CPython does it.
    char s[1];
    s[0] = (char)c;

#if PYTHON_VERSION < 300
    return PyString_FromStringAndSize( s, 1 );
#else
    return PyUnicode_FromStringAndSize( s, 1 );
#endif
}

PyObject *BUILTIN_CHR( PyObject *value )
{
    long x = PyInt_AsLong( value );

    if ( x < 0 || x >= 256 )
    {
        PyErr_Format( PyExc_ValueError, "chr() arg not in range(256)" );
        throw _PythonException();
    }

    // TODO: A switch statement might be faster, because no object needs to be created at
    // all, this is how CPython does it.
    char s[1];
    s[0] = (char)x;

#if PYTHON_VERSION < 300
    return PyString_FromStringAndSize( s, 1 );
#else
    return PyUnicode_FromStringAndSize( s, 1 );
#endif
}

PyObject *BUILTIN_ORD( PyObject *value )
{
    long result;

    if (likely( PyBytes_Check( value ) ))
    {
        Py_ssize_t size = PyBytes_GET_SIZE( value );

        if (likely( size == 1 ))
        {
            result = long( ((unsigned char *)PyBytes_AS_STRING( value ))[0] );
        }
        else
        {
            PyErr_Format( PyExc_TypeError, "ord() expected a character, but string of length %" PY_FORMAT_SIZE_T "d found", size );
            throw _PythonException();
        }
    }
    else if ( PyByteArray_Check( value ) )
    {
        Py_ssize_t size = PyByteArray_GET_SIZE( value );

        if (likely( size == 1 ))
        {
            result = long( ((unsigned char *)PyByteArray_AS_STRING( value ))[0] );
        }
        else
        {
            PyErr_Format( PyExc_TypeError, "ord() expected a character, but byte array of length %" PY_FORMAT_SIZE_T "d found", size );
            throw _PythonException();
        }
    }
    else if ( PyUnicode_Check( value ) )
    {
        Py_ssize_t size = PyUnicode_GET_SIZE( value );

        if (likely( size == 1 ))
        {
            result = long( *PyUnicode_AS_UNICODE( value ) );
        }
        else
        {
            PyErr_Format( PyExc_TypeError, "ord() expected a character, but unicode string of length %" PY_FORMAT_SIZE_T "d found", size );
            throw _PythonException();
        }
    }
    else
    {
        PyErr_Format( PyExc_TypeError, "ord() expected string of length 1, but %s found", Py_TYPE( value )->tp_name );
        throw _PythonException();
    }

    return PyInt_FromLong( result );
}

PyObject *BUILTIN_BIN( PyObject *value )
{
    // Note: I don't really know why ord and hex don't use this as well.
    PyObject *result = PyNumber_ToBase( value, 2 );

    if ( unlikely( result == NULL ))
    {
        throw _PythonException();
    }

    return result;
}

PyObject *BUILTIN_OCT( PyObject *value )
{
#if PYTHON_VERSION >= 300
    PyObject *result = PyNumber_ToBase( value, 8 );

    if ( unlikely( result == NULL ))
    {
        throw _PythonException();
    }

    return result;
#else
    if (unlikely( value == NULL ))
    {
        PyErr_Format( PyExc_TypeError, "oct() argument can't be converted to oct" );
        return NULL;
    }

    PyNumberMethods *nb = Py_TYPE( value )->tp_as_number;

    if (unlikely( nb == NULL || nb->nb_oct == NULL ))
    {
        PyErr_Format( PyExc_TypeError, "oct() argument can't be converted to oct" );
        return NULL;
    }

    PyObject *result = (*nb->nb_oct)( value );

    if ( result )
    {
        if (unlikely( !PyString_Check( result ) ))
        {
            PyErr_Format( PyExc_TypeError, "__oct__ returned non-string (type %s)", Py_TYPE( result )->tp_name );

            Py_DECREF( result );
            return NULL;
        }
    }

    return result;
#endif
}

PyObject *BUILTIN_HEX( PyObject *value )
{
#if PYTHON_VERSION >= 300
    PyObject *result = PyNumber_ToBase( value, 16 );

    if ( unlikely( result == NULL ))
    {
        throw _PythonException();
    }

    return result;
#else
    if (unlikely( value == NULL ))
    {
        PyErr_Format( PyExc_TypeError, "hex() argument can't be converted to hex" );
        return NULL;
    }

    PyNumberMethods *nb = Py_TYPE( value )->tp_as_number;

    if (unlikely( nb == NULL || nb->nb_hex == NULL ))
    {
        PyErr_Format( PyExc_TypeError, "hex() argument can't be converted to hex" );
        return NULL;
    }

    PyObject *result = (*nb->nb_hex)( value );

    if ( result )
    {
        if (unlikely( !PyString_Check( result ) ))
        {
            PyErr_Format( PyExc_TypeError, "__hex__ returned non-string (type %s)", Py_TYPE( result )->tp_name );

            Py_DECREF( result );
            return NULL;
        }
    }

    return result;
#endif
}

// From CPython:
typedef struct {
    PyObject_HEAD
    PyObject *it_callable;
    PyObject *it_sentinel;
} calliterobject;

PyObject *BUILTIN_ITER2( PyObject *callable, PyObject *sentinel )
{
    calliterobject *result = PyObject_GC_New( calliterobject, &PyCallIter_Type );

    if (unlikely( result == NULL ))
    {
        throw _PythonException();
    }

    // Note: References were taken at call site already.
    result->it_callable = callable;
    result->it_sentinel = sentinel;

    Nuitka_GC_Track( result );

    return (PyObject *)result;
}

PyObject *BUILTIN_TYPE1( PyObject *arg )
{
    return INCREASE_REFCOUNT( (PyObject *)Py_TYPE( arg ) );
}

extern PyObject *_python_str_plain___module__;

PyObject *BUILTIN_TYPE3( PyObject *module_name, PyObject *name, PyObject *bases, PyObject *dict )
{
    PyObject *result = PyType_Type.tp_new(
        &PyType_Type,
        PyObjectTemporary( MAKE_TUPLE3( name, bases, dict ) ).asObject(),
        NULL
    );

    if (unlikely( result == NULL ))
    {
        throw _PythonException();
    }

    PyTypeObject *type = Py_TYPE( result );

    if (likely( PyType_IsSubtype( type, &PyType_Type ) ))
    {
        if (
#if PYTHON_VERSION < 300
            PyType_HasFeature( type, Py_TPFLAGS_HAVE_CLASS ) &&
#endif
            type->tp_init != NULL
           )
        {
            int res = type->tp_init( result, MAKE_TUPLE3( name, bases, dict ), NULL );

            if (unlikely( res < 0 ))
            {
                Py_DECREF( result );
                throw _PythonException();
            }
        }
    }

    int res = PyObject_SetAttr( result, _python_str_plain___module__, module_name );

    if ( res == -1 )
    {
        throw _PythonException();
    }

    return result;
}

Py_ssize_t ESTIMATE_RANGE( long low, long high, long step )
{
    if ( low >= high )
    {
        return 0;
    }
    else
    {
        return ( high - low - 1 ) / step + 1;
    }
}

PyObject *BUILTIN_RANGE( long low, long high, long step )
{
    assert( step != 0 );

    Py_ssize_t size;

    if ( step > 0 )
    {
        size = ESTIMATE_RANGE( low, high, step );
    }
    else
    {
        size = ESTIMATE_RANGE( high, low, -step );
    }

    PyObject *result = PyList_New( size );

    long current = low;

    for( int i = 0; i < size; i++ )
    {
        PyList_SET_ITEM( result, i, PyInt_FromLong( current ) );
        current += step;
    }

    return result;
}

PyObject *BUILTIN_RANGE( long low, long high )
{
    return BUILTIN_RANGE( low, high, 1 );
}

PyObject *BUILTIN_RANGE( long boundary )
{
    return BUILTIN_RANGE( 0, boundary );
}

#if PYTHON_VERSION < 300
static PyObject *TO_RANGE_ARG( PyObject *value, char const *name )
{
    if (likely(
#if PYTHON_VERSION < 300
            PyInt_Check( value ) ||
#endif
            PyLong_Check( value )
       ))
    {
        return INCREASE_REFCOUNT( value );
    }

    PyTypeObject *type = Py_TYPE( value );
    PyNumberMethods *tp_as_number = type->tp_as_number;

    // Everything that casts to int is allowed.
    if (
#if PYTHON_VERSION >= 270
        PyFloat_Check( value ) ||
#endif
        tp_as_number == NULL ||
        tp_as_number->nb_int == NULL
       )
    {
        PyErr_Format( PyExc_TypeError, "range() integer %s argument expected, got %s.", name, type->tp_name );
        throw _PythonException();
    }

    PyObject *result = tp_as_number->nb_int( value );

    if (unlikely( result == NULL ))
    {
        throw _PythonException();
    }

    return result;
}
#endif

extern PyObject *_python_str_plain_range;

static PythonBuiltin _python_builtin_range( &_python_str_plain_range );

PyObject *BUILTIN_RANGE( PyObject *boundary )
{
#if PYTHON_VERSION < 300
    PyObjectTemporary boundary_temp( TO_RANGE_ARG( boundary, "end" ) );

    long start = PyInt_AsLong( boundary_temp.asObject() );

    if ( start == -1 && ERROR_OCCURED() )
    {
        PyErr_Clear();

        return _python_builtin_range.call1( boundary_temp.asObject() );
    }

    return BUILTIN_RANGE( start );
#else
    return _python_builtin_range.call1( boundary );
#endif
}

PyObject *BUILTIN_RANGE( PyObject *low, PyObject *high )
{
#if PYTHON_VERSION < 300
    PyObjectTemporary low_temp( TO_RANGE_ARG( low, "start" ) );
    PyObjectTemporary high_temp( TO_RANGE_ARG( high, "end" ) );

    bool fallback = false;

    long start = PyInt_AsLong( low_temp.asObject() );

    if (unlikely( start == -1 && ERROR_OCCURED() ))
    {
        PyErr_Clear();
        fallback = true;
    }

    long end = PyInt_AsLong( high_temp.asObject() );

    if (unlikely( end == -1 && ERROR_OCCURED() ))
    {
        PyErr_Clear();
        fallback = true;
    }

    if ( fallback )
    {
        return _python_builtin_range.call_args(
            MAKE_TUPLE2(
                low_temp.asObject(),
                high_temp.asObject()
            )
        );
    }
    else
    {
        return BUILTIN_RANGE( start, end );
    }
#else
    return _python_builtin_range.call_args(
        MAKE_TUPLE2( low, high )
    );
#endif
}

PyObject *BUILTIN_RANGE( PyObject *low, PyObject *high, PyObject *step )
{
#if PYTHON_VERSION < 300
    PyObjectTemporary low_temp( TO_RANGE_ARG( low, "start" ) );
    PyObjectTemporary high_temp( TO_RANGE_ARG( high, "end" ) );
    PyObjectTemporary step_temp( TO_RANGE_ARG( step, "step" ) );

    bool fallback = false;

    long start = PyInt_AsLong( low_temp.asObject() );

    if (unlikely( start == -1 && ERROR_OCCURED() ))
    {
        PyErr_Clear();
        fallback = true;
    }

    long end = PyInt_AsLong( high_temp.asObject() );

    if (unlikely( end == -1 && ERROR_OCCURED() ))
    {
        PyErr_Clear();
        fallback = true;
    }

    long step_long = PyInt_AsLong( step_temp.asObject() );

    if (unlikely( step_long == -1 && ERROR_OCCURED() ))
    {
        PyErr_Clear();
        fallback = true;
    }

    if ( fallback )
    {
        return _python_builtin_range.call_args(
            MAKE_TUPLE3(
                low_temp.asObject(),
                high_temp.asObject(),
                step_temp.asObject()
            )
       );
    }
    else
    {
        if (unlikely( step_long == 0 ))
        {
            PyErr_Format( PyExc_ValueError, "range() step argument must not be zero" );
            throw _PythonException();
        }

        return BUILTIN_RANGE( start, end, step_long );
    }
#else
    return _python_builtin_range.call_args(
        MAKE_TUPLE3( low, high, step )
    );
#endif
}

PyObject *BUILTIN_LEN( PyObject *value )
{
    assertObject( value );

    Py_ssize_t res = PyObject_Size( value );

    if (unlikely( res < 0 && ERROR_OCCURED() ))
    {
        throw _PythonException();
    }

    return PyInt_FromSsize_t( res );
}

PyObject *BUILTIN_DIR1( PyObject *arg )
{
    assertObject( arg );

    PyObject *result = PyObject_Dir( arg );

    if (unlikely( result == NULL ))
    {
        throw _PythonException();
    }

    return result;
}

extern PyObject *_python_str_empty;
extern PyObject *_python_bytes_empty;

PyCodeObject *MAKE_CODEOBJ( PyObject *filename, PyObject *function_name, int line, PyObject *argnames, int arg_count, bool is_generator )
{
    assertObject( filename );
    assert( Nuitka_String_Check( filename ) );
    assertObject( function_name );
    assert( Nuitka_String_Check( function_name ) );
    assertObject( argnames );
    assert( PyTuple_Check( argnames ) );

    int flags = 0;

    if ( is_generator )
    {
        flags |= CO_GENERATOR;
    }

    // TODO: Consider using PyCode_NewEmpty

    PyCodeObject *result = PyCode_New (
        arg_count,           // argcount
#if PYTHON_VERSION >= 300
        0,                   // kw-only count
#endif
        0,                   // nlocals
        0,                   // stacksize
        flags,               // flags
#if PYTHON_VERSION < 300
        _python_str_empty,   // code (bytecode)
#else
        _python_bytes_empty, // code (bytecode)
#endif
        _python_tuple_empty, // consts (we are not going to be compatible)
        _python_tuple_empty, // names (we are not going to be compatible)
        argnames,            // varnames (we are not going to be compatible)
        _python_tuple_empty, // freevars (we are not going to be compatible)
        _python_tuple_empty, // cellvars (we are not going to be compatible)
        filename,            // filename
        function_name,       // name
        line,                // firstlineno (offset of the code object)
#if PYTHON_VERSION < 300
        _python_str_empty    // lnotab (table to translate code object)
#else
        _python_bytes_empty  // lnotab (table to translate code object)
#endif
    );

    if (unlikely( result == NULL ))
    {
        throw _PythonException();
    }

    return result;
}

extern PyObject *_python_dict_empty;

PyFrameObject *MAKE_FRAME( PyCodeObject *code, PyObject *module )
{
    assertCodeObject( code );
    assertObject( module );

    PyFrameObject *current = PyThreadState_GET()->frame;

    PyFrameObject *result = PyFrame_New(
        PyThreadState_GET(),                 // thread state
        code,                                // code
        ((PyModuleObject *)module)->md_dict, // globals (module dict)
        _python_dict_empty                   // locals (we are not going to be compatible (yet?))
    );

    assertCodeObject( code );

    if (unlikely( result == NULL ))
    {
        throw _PythonException();
    }

    assert( current == PyThreadState_GET()->frame );

    // Provide a non-NULL f_trace, so f_lineno will be used in exceptions.
    result->f_trace = INCREASE_REFCOUNT( Py_None );

    // Remove the reference to the current frame, to be set when actually using it
    // only.
    Py_XDECREF( result->f_back );
    result->f_back = NULL;

    return result;
}

static PyFrameObject *duplicateFrame( PyFrameObject *old_frame )
{
    PyFrameObject *new_frame = PyObject_GC_NewVar( PyFrameObject, &PyFrame_Type, 0 );

    // Allow only to detach only our tracing frames.
    assert( old_frame->f_trace == Py_None );
    new_frame->f_trace = INCREASE_REFCOUNT( Py_None );

    // Copy the back reference if any.
    new_frame->f_back = old_frame->f_back;
    Py_XINCREF( new_frame->f_back );

    // Take a code reference as well.
    new_frame->f_code = old_frame->f_code;
    Py_XINCREF( new_frame->f_code );

    // Copy attributes.
    new_frame->f_locals = INCREASE_REFCOUNT( old_frame->f_locals );
    new_frame->f_globals = INCREASE_REFCOUNT( old_frame->f_globals );
    new_frame->f_builtins = INCREASE_REFCOUNT( old_frame->f_builtins );

    new_frame->f_exc_type = INCREASE_REFCOUNT_X( old_frame->f_exc_type );
    new_frame->f_exc_value = INCREASE_REFCOUNT_X( old_frame->f_exc_value );
    new_frame->f_exc_traceback = INCREASE_REFCOUNT_X( old_frame->f_exc_traceback );

    assert( old_frame->f_valuestack == old_frame->f_localsplus );
    new_frame->f_valuestack = new_frame->f_localsplus;

    assert( old_frame->f_stacktop == old_frame->f_valuestack );
    new_frame->f_stacktop = new_frame->f_valuestack;

    new_frame->f_tstate = old_frame->f_tstate;
    new_frame->f_lasti = -1;
    new_frame->f_lineno = old_frame->f_lineno;

    assert( old_frame->f_iblock == 0 );
    new_frame->f_iblock = 0;

    Nuitka_GC_Track( new_frame );

    return new_frame;
}

PyFrameObject *detachCurrentFrame()
{
    PyFrameObject *old_frame = PyThreadState_GET()->frame;

    // Duplicate it.
    PyFrameObject *new_frame = duplicateFrame( old_frame );

    // The given frame can be put on top now.
    PyThreadState_GET()->frame = new_frame;
    Py_DECREF( old_frame );

    return new_frame;
}

extern PyObject *_python_str_plain___import__;

static PythonBuiltin _python_builtin_import( &_python_str_plain___import__ );

PyObject *IMPORT_MODULE( PyObject *module_name, PyObject *globals, PyObject *locals, PyObject *import_items, PyObject *level )
{
    assert( Nuitka_String_Check( module_name ) );
    assertObject( globals );
    assertObject( locals );
    assertObject( import_items );

    PyObject *import_result;

    _python_builtin_import.refresh();

    import_result = _python_builtin_import.call_args(
        MAKE_TUPLE5(
            module_name,
            globals,
            locals,
            import_items,
            level
        )
    );

    return import_result;
}

void IMPORT_MODULE_STAR( PyObject *target, bool is_module, PyObject *module )
{
    // Check parameters.
    assertObject( module );
    assertObject( target );

    PyObject *iter;
    bool all_case;

    if ( PyObject *all = PyObject_GetAttrString( module, (char *)"__all__" ) )
    {
        iter = MAKE_ITERATOR( all );
        all_case = true;
    }
    else if ( PyErr_ExceptionMatches( PyExc_AttributeError ) )
    {
        PyErr_Clear();

        iter = MAKE_ITERATOR( PyModule_GetDict( module ) );
        all_case = false;
    }
    else
    {
        throw _PythonException();
    }

    while ( PyObject *item = ITERATOR_NEXT( iter ) )
    {
        assert( Nuitka_String_Check( item ) );

        // TODO: Not yet clear, what happens with __all__ and "_" of its contents.
        if ( all_case == false )
        {
            if ( Nuitka_String_AsString_Unchecked( item )[0] == '_' )
            {
                continue;
            }
        }

        // TODO: Check if the reference is handled correctly
        if ( is_module )
        {
            SET_ATTRIBUTE( target, item, LOOKUP_ATTRIBUTE( module, item ) );
        }
        else
        {
            SET_SUBSCRIPT( LOOKUP_ATTRIBUTE( module, item ), target, item );
        }

        Py_DECREF( item );
    }
}

// Helper functions for print. Need to play nice with Python softspace behaviour.

extern PyObject *_python_str_plain_print;
extern PyObject *_python_str_plain_end;
extern PyObject *_python_str_plain_file;

static PythonBuiltin _python_builtin_print( &_python_str_plain_print );

void PRINT_ITEM_TO( PyObject *file, PyObject *object )
{
// The print builtin function cannot replace "softspace" behaviour of CPython
// print statement, so this code is really necessary.
#if PYTHON_VERSION < 300
    if ( file == NULL || file == Py_None )
    {
        file = GET_STDOUT();
    }

    assertObject( file );
    assertObject( object );

    // need to hold a reference to the file or else __getattr__ may release "file" in the
    // mean time.
    Py_INCREF( file );

    PyObject *str = PyObject_Str( object );
    PyObject *print;
    bool softspace;

    if ( str == NULL )
    {
        PyErr_Clear();

        print = object;
        softspace = false;
    }
    else
    {
        char *buffer;
        Py_ssize_t length;

#ifndef __NUITKA_NO_ASSERT__
        int status =
#endif
            PyString_AsStringAndSize( str, &buffer, &length );
        assert( status != -1 );

        softspace = length > 0 && buffer[ length - 1 ] == '\t';

        print = str;
    }

    // Check for soft space indicator
    if ( PyFile_SoftSpace( file, !softspace ) )
    {
        if (unlikely( PyFile_WriteString( " ", file ) == -1 ))
        {
            Py_DECREF( file );
            Py_DECREF( str );
            throw _PythonException();
        }
    }

    if ( unlikely( PyFile_WriteObject( print, file, Py_PRINT_RAW ) == -1 ))
    {
        Py_DECREF( file );
        Py_XDECREF( str );
        throw _PythonException();
    }

    Py_XDECREF( str );

    if ( softspace )
    {
        PyFile_SoftSpace( file, !softspace );
    }

    assertObject( file );

    Py_DECREF( file );
#else
    _python_builtin_print.refresh();

    if (likely( file == NULL ))
    {
        _python_builtin_print.call1(
            object
        );
    }
    else
    {
        PyObjectTemporary print_kw(
            MAKE_DICT2(
                _python_str_empty, _python_str_plain_end,
                GET_STDOUT(), _python_str_plain_file
            )
        );

        PyObjectTemporary print_args(
            MAKE_TUPLE1( object )
        );

        _python_builtin_print.call_args_kw(
            print_args.asObject(),
            print_kw.asObject()
        );
    }
#endif
}

void PRINT_NEW_LINE_TO( PyObject *file )
{
#if PYTHON_VERSION < 300
    if ( file == NULL || file == Py_None )
    {
        file = GET_STDOUT();
    }

    // need to hold a reference to the file or else __getattr__ may release "file" in the
    // mean time.
    Py_INCREF( file );

    if (unlikely( PyFile_WriteString( "\n", file ) == -1))
    {
        Py_DECREF( file );
        throw _PythonException();
    }

    PyFile_SoftSpace( file, 0 );

    assertObject( file );

    Py_DECREF( file );
#else
    if (likely( file == NULL ))
    {
        _python_builtin_print.call();
    }
    else
    {
        PyObjectTemporary print_keyargs(
            MAKE_DICT1( // Note: Values for first for MAKE_DICT
                GET_STDOUT(), _python_str_plain_file
            )
        );

        _python_builtin_print.call_kw(
            print_keyargs.asObject()
        );
    }
#endif
}

void PRINT_REFCOUNT( PyObject *object )
{
#if PYTHON_VERSION < 300
   char buffer[ 1024 ];
   sprintf( buffer, " refcnt %" PY_FORMAT_SIZE_T "d ", Py_REFCNT( object ) );

   if (unlikely( PyFile_WriteString( buffer, GET_STDOUT() ) == -1 ))
   {
      throw _PythonException();
   }
#else
   assert( false );
#endif
}




PyObject *GET_STDOUT()
{
    PyObject *result = PySys_GetObject( (char *)"stdout" );

    if (unlikely( result == NULL ))
    {
        PyErr_Format( PyExc_RuntimeError, "lost sys.stdout" );
        throw _PythonException();
    }

    return result;
}

PyObject *GET_STDERR()
{
    PyObject *result = PySys_GetObject( (char *)"stderr" );

    if (unlikely( result == NULL ))
    {
        PyErr_Format( PyExc_RuntimeError, "lost sys.stderr" );
        throw _PythonException();
    }

    return result;
}

#if PYTHON_VERSION < 300

void PRINT_NEW_LINE( void )
{
    PRINT_NEW_LINE_TO( GET_STDOUT() );
}

#endif

// We unstream some constant objects using the "cPickle" module function "loads"
static PyObject *_module_cPickle = NULL;
static PyObject *_module_cPickle_function_loads = NULL;

void UNSTREAM_INIT( void )
{
#if PYTHON_VERSION < 300
    _module_cPickle = PyImport_ImportModule( "cPickle" );
#else
    _module_cPickle = PyImport_ImportModule( "pickle" );
#endif
    assert( _module_cPickle );

    _module_cPickle_function_loads = PyObject_GetAttrString( _module_cPickle, "loads" );
    assert( _module_cPickle_function_loads );
}

PyObject *UNSTREAM_CONSTANT( char const *buffer, Py_ssize_t size )
{
    PyObject *result = PyObject_CallFunction(
        _module_cPickle_function_loads,
#if PYTHON_VERSION < 300
        (char *)"(s#)",
#else
        (char *)"(y#)",
#endif
        buffer,
        size
    );

    if ( !result )
    {
        PyErr_Print();
    }

    assertObject( result );

    return result;
}

PyObject *UNSTREAM_STRING( char const *buffer, Py_ssize_t size, bool intern )
{
#if PYTHON_VERSION < 300
    PyObject *result = PyString_FromStringAndSize( buffer, size );
#else
    PyObject *result = PyUnicode_FromStringAndSize( buffer, size );
#endif
    assert( !ERROR_OCCURED() );
    assertObject( result );
    assert( Nuitka_String_Check( result ) );

#if PYTHON_VERSION < 300
    assert( PyString_Size( result ) == size );
#else
    assert( PyUnicode_GET_SIZE( result ) == size );
#endif

    if ( intern )
    {
#if PYTHON_VERSION < 300
        PyString_InternInPlace( &result );
#else
        PyUnicode_InternInPlace( &result );
#endif
        assertObject( result );
        assert( Nuitka_String_Check( result ) );

#if PYTHON_VERSION < 300
        assert( PyString_Size( result ) == size );
#else
        assert( PyUnicode_GET_SIZE( result ) == size );
#endif
    }

    return result;
}

#if PYTHON_VERSION < 300

static void set_slot( PyObject **slot, PyObject *value )
{
    PyObject *temp = *slot;
    Py_XINCREF( value );
    *slot = value;
    Py_XDECREF( temp );
}

static void set_attr_slots( PyClassObject *klass )
{
    static PyObject *getattrstr = NULL, *setattrstr = NULL, *delattrstr = NULL;

    if ( getattrstr == NULL )
    {
        getattrstr = PyString_InternFromString( "__getattr__" );
        setattrstr = PyString_InternFromString( "__setattr__" );
        delattrstr = PyString_InternFromString( "__delattr__" );
    }


    set_slot( &klass->cl_getattr, FIND_ATTRIBUTE_IN_CLASS( klass, getattrstr ) );
    set_slot( &klass->cl_setattr, FIND_ATTRIBUTE_IN_CLASS( klass, setattrstr ) );
    set_slot( &klass->cl_delattr, FIND_ATTRIBUTE_IN_CLASS( klass, delattrstr ) );
}

static bool set_dict( PyClassObject *klass, PyObject *value )
{
    if ( value == NULL || !PyDict_Check( value ) )
    {
        PyErr_SetString( PyExc_TypeError, (char *)"__dict__ must be a dictionary object" );
        return false;
    }
    else
    {
        set_slot( &klass->cl_dict, value );
        set_attr_slots( klass );

        return true;
    }
}

static bool set_bases( PyClassObject *klass, PyObject *value )
{
    if ( value == NULL || !PyTuple_Check( value ) )
    {
        PyErr_SetString( PyExc_TypeError, (char *)"__bases__ must be a tuple object" );
        return false;
    }
    else
    {
        Py_ssize_t n = PyTuple_Size( value );

        for ( Py_ssize_t i = 0; i < n; i++ )
        {
            PyObject *base = PyTuple_GET_ITEM( value, i );

            if (unlikely( !PyClass_Check( base ) ))
            {
                PyErr_SetString( PyExc_TypeError, (char *)"__bases__ items must be classes" );
                return false;
            }

            if (unlikely( PyClass_IsSubclass( base, (PyObject *)klass) ))
            {
                PyErr_SetString( PyExc_TypeError, (char *)"a __bases__ item causes an inheritance cycle" );
                return false;
            }
        }

        set_slot( &klass->cl_bases, value );
        set_attr_slots( klass );

        return true;
    }
}

static bool set_name( PyClassObject *klass, PyObject *value )
{
    if ( value == NULL || !PyDict_Check( value ) )
    {
        PyErr_SetString( PyExc_TypeError, (char *)"__name__ must be a string object" );
        return false;
    }

    if ( strlen( PyString_AS_STRING( value )) != (size_t)PyString_GET_SIZE( value ) )
    {
        PyErr_SetString( PyExc_TypeError, (char *)"__name__ must not contain null bytes" );
        return false;
    }

    set_slot( &klass->cl_name, value );
    return true;
}

static int nuitka_class_setattr( PyClassObject *klass, PyObject *attr_name, PyObject *value )
{
    char *sattr_name = PyString_AsString( attr_name );

    if ( sattr_name[0] == '_' && sattr_name[1] == '_' )
    {
        Py_ssize_t n = PyString_Size( attr_name );

        if ( sattr_name[ n-2 ] == '_' && sattr_name[ n-1 ] == '_' )
        {
            if ( strcmp( sattr_name, "__dict__" ) == 0 )
            {
                if ( set_dict( klass, value ) == false )
                {
                    return -1;
                }
                else
                {
                    return 0;
                }
            }
            else if ( strcmp( sattr_name, "__bases__" ) == 0 )
            {
                if ( set_bases( klass, value ) == false )
                {
                    return -1;
                }
                else
                {
                    return 0;
                }
            }
            else if ( strcmp( sattr_name, "__name__" ) == 0 )
            {
                if ( set_name( klass, value ) == false )
                {
                    return -1;
                }
                else
                {
                    return 0;
                }
            }
            else if ( strcmp( sattr_name, "__getattr__" ) == 0 )
            {
                set_slot( &klass->cl_getattr, value );
            }
            else if ( strcmp(sattr_name, "__setattr__" ) == 0 )
            {
                set_slot( &klass->cl_setattr, value );
            }
            else if ( strcmp(sattr_name, "__delattr__" ) == 0 )
            {
                set_slot( &klass->cl_delattr, value );
            }
        }
    }

    if ( value == NULL )
    {
        int status = PyDict_DelItem( klass->cl_dict, attr_name );

        if ( status < 0 )
        {
            PyErr_Format( PyExc_AttributeError, "class %s has no attribute '%s'", PyString_AS_STRING( klass->cl_name ), sattr_name );
        }

        return status;
    }
    else
    {
        return PyDict_SetItem( klass->cl_dict, attr_name, value );
    }
}

static PyObject *nuitka_class_getattr( PyClassObject *klass, PyObject *attr_name )
{
    char *sattr_name = PyString_AsString( attr_name );

    if ( sattr_name[0] == '_' && sattr_name[1] == '_' )
    {
        if ( strcmp( sattr_name, "__dict__" ) == 0 )
        {
            return INCREASE_REFCOUNT( klass->cl_dict );
        }
        else if ( strcmp(sattr_name, "__bases__" ) == 0 )
        {
            return INCREASE_REFCOUNT( klass->cl_bases );
        }
        else if ( strcmp(sattr_name, "__name__" ) == 0 )
        {
            return klass->cl_name ? INCREASE_REFCOUNT( klass->cl_name ) : INCREASE_REFCOUNT( Py_None );
        }
    }

    PyObject *value = FIND_ATTRIBUTE_IN_CLASS( klass, attr_name );

    if (unlikely( value == NULL ))
    {
        PyErr_Format( PyExc_AttributeError, "class %s has no attribute '%s'", PyString_AS_STRING( klass->cl_name ), sattr_name );
        return NULL;
    }

    PyTypeObject *type = Py_TYPE( value );

    descrgetfunc tp_descr_get = PyType_HasFeature( type, Py_TPFLAGS_HAVE_CLASS ) ? type->tp_descr_get : NULL;

    if ( tp_descr_get == NULL )
    {
        return INCREASE_REFCOUNT( value );
    }
    else
    {
        return tp_descr_get( value, (PyObject *)NULL, (PyObject *)klass );
    }
}

#endif

void enhancePythonTypes( void )
{
#if PYTHON_VERSION < 300
    // Our own variant won't call PyEval_GetRestricted, saving quite some cycles not doing
    // that.
    PyClass_Type.tp_setattro = (setattrofunc)nuitka_class_setattr;
    PyClass_Type.tp_getattro = (getattrofunc)nuitka_class_getattr;
#endif
}

#ifdef __APPLE__
extern wchar_t* _Py_DecodeUTF8_surrogateescape(const char *s, Py_ssize_t size);
#endif

#ifdef __FreeBSD__
#include <floatingpoint.h>
#endif

#include <locale.h>

void setCommandLineParameters( int argc, char *argv[] )
{
#if PYTHON_VERSION < 300
    PySys_SetArgv( argc, argv );
#else
// Originally taken from CPython3: There seems to be no sane way to use

    wchar_t **argv_copy = (wchar_t **)PyMem_Malloc(sizeof(wchar_t*)*argc);
    /* We need a second copies, as Python might modify the first one. */
    wchar_t **argv_copy2 = (wchar_t **)PyMem_Malloc(sizeof(wchar_t*)*argc);

    char *oldloc;
    /* 754 requires that FP exceptions run in "no stop" mode by default,
     * and until C vendors implement C99's ways to control FP exceptions,
     * Python requires non-stop mode.  Alas, some platforms enable FP
     * exceptions by default.  Here we disable them.
     */
#ifdef __FreeBSD__
    fp_except_t m;

    m = fpgetmask();
    fpsetmask(m & ~FP_X_OFL);
#endif

    oldloc = strdup( setlocale( LC_ALL, NULL ) );

    setlocale( LC_ALL, "" );
    for ( int i = 0; i < argc; i++ )
    {
#ifdef __APPLE__
        argv_copy[i] = _Py_DecodeUTF8_surrogateescape( argv[ i ], strlen( argv[ i ] ) );
#else
        argv_copy[i] = _Py_char2wchar( argv[ i ], NULL );
#endif
        assert ( argv_copy[ i ] );

        argv_copy2[ i ] = argv_copy[ i ];
    }
    setlocale( LC_ALL, oldloc );
    free( oldloc );

    PySys_SetArgv( argc, argv_copy );
#endif
}

typedef struct {
    PyObject_HEAD
    PyTypeObject *type;
    PyObject *obj;
    PyTypeObject *obj_type;
} superobject;

extern PyObject *_python_str_plain___class__;

PyObject *BUILTIN_SUPER( PyObject *type, PyObject *object )
{
    assertObject( type );
    assert( PyType_Check( type ));

    superobject *result = PyObject_GC_New( superobject, &PySuper_Type );
    assert( result );

    result->type = (PyTypeObject *)INCREASE_REFCOUNT( type );
    if ( object )
    {
        result->obj = INCREASE_REFCOUNT( object );

        if ( PyType_Check( object ) && PyType_IsSubtype( (PyTypeObject *)object, (PyTypeObject *)type ))
        {
            result->obj_type = (PyTypeObject *)INCREASE_REFCOUNT( object );
        }
        else if ( PyType_IsSubtype( Py_TYPE(object ), (PyTypeObject *)type) )
        {
            result->obj_type = (PyTypeObject *)INCREASE_REFCOUNT( (PyObject *)Py_TYPE( object ) );
        }
        else
        {
            PyObject *class_attr = PyObject_GetAttr( object, _python_str_plain___class__);

            if ( class_attr != NULL && PyType_Check( class_attr ) && (PyTypeObject *)class_attr != Py_TYPE( object ) )
            {
                result->obj_type = (PyTypeObject *)class_attr;
            }
            else
            {
                if ( class_attr == NULL )
                {
                    PyErr_Clear();
                }
                else
                {
                    Py_DECREF( class_attr );
                }

                PyErr_Format(
                    PyExc_TypeError,
                    "super(type, obj): obj must be an instance or subtype of type"
                );

                throw _PythonException();
            }
        }
    }
    else
    {
        result->obj = NULL;
        result->obj_type = NULL;
    }

    Nuitka_GC_Track( result );

    assertObject( (PyObject *)result );

    assert( Py_TYPE( result ) == &PySuper_Type );
    Py_INCREF( result );

    return (PyObject *)result;
}

extern PyObject *_python_str_plain___metaclass__;
extern PyObject *_python_str_plain___class__;

PyObject *_MAKE_CLASS( EVAL_ORDERED_5( PyObject *metaclass_global, PyObject *metaclass_class, PyObject *class_name, PyObject *bases, PyObject *class_dict ) )
{
    // This selection is dynamic, although it is something that might be determined at
    // compile time already in many cases, and therefore should be a function that is
    // built of nodes.
#if PYTHON_VERSION < 300
    PyObject *metaclass;

    if ( metaclass_class != NULL )
    {
        metaclass = metaclass_class;
    }
    else
    {
        metaclass = PyDict_GetItem( class_dict, _python_str_plain___metaclass__ );
    }

    // Prefer the metaclass entry of the new class, otherwise search the base classes for
    // their metaclass.
    if ( metaclass != NULL )
    {
        /* Hold a reference to the metaclass while we use it. */
        Py_INCREF( metaclass );
    }
    else
#else
    PyObject *metaclass = metaclass_class;

    if ( metaclass == NULL )
#endif
    {
        assertObject( bases );

        if ( PyTuple_GET_SIZE( bases ) > 0 )
        {
            PyObject *base = PyTuple_GET_ITEM( bases, 0 );

            metaclass = PyObject_GetAttr( base, _python_str_plain___class__ );

            if ( metaclass == NULL )
            {
                PyErr_Clear();

                metaclass = INCREASE_REFCOUNT( (PyObject *)Py_TYPE( base ) );
            }
        }
        else if ( metaclass_global != NULL )
        {
            metaclass = INCREASE_REFCOUNT( metaclass_global );
        }
        else
        {
#if PYTHON_VERSION < 300
            // Default to old style class.
            metaclass = INCREASE_REFCOUNT( (PyObject *)&PyClass_Type );
#else
            // Default to new style class.
            metaclass = INCREASE_REFCOUNT( (PyObject *)&PyType_Type );
#endif
        }
    }

    PyObject *result = PyObject_CallFunctionObjArgs(
        metaclass,
        class_name,
        bases,
        class_dict,
        NULL
    );

    Py_DECREF( metaclass );

    if (unlikely( result == NULL ))
    {
        throw _PythonException();
    }

    return result;
}
