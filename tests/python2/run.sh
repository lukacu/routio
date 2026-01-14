
# Proof-of-concept test for different Python version communication

routio-router

docker run -ti --rm -v  "`realpath tests/python2/script.py`:/script.py:ro" -v /tmp/echo.sock:/tmp/echo.sock routio:old python script.py a b

docker run -ti --rm -v  "`realpath tests/python2/script.py`:/script.py:ro" -v /tmp/echo.sock:/tmp/echo.sock routio python script.py b a