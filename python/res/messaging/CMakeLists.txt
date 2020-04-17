set(PYTHON_SOURCES
    __init__.py
    client.py
    server.py)

add_python_package("python.res.messaging" ${PYTHON_INSTALL_PREFIX}/res/messaging "${PYTHON_SOURCES}" True)
