# for calling dos upload from Window WSL2 Linux, because serial ports are not mapped yet
Import('env', "projenv")
from subprocess import call

def upload(source, target, env):

    print("bin file: " + str(target[0]))

    # esp8266 or esp32
    platform = "esp" + env['PIOPLATFORM'].strip("espressif")

    if platform == 'esp8266':
        call(["cmd.exe", "/c", "C:\\Users\\Paul\\OneDrive\\Desktop\\esp8266_heaptest.bat"])

    if platform == 'esp32':
        call(["cmd.exe", "/c", "C:\\Users\\Paul\\OneDrive\\Desktop\\esp32_heaptest.bat"])

env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", [upload])
