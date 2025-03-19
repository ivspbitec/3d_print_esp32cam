Import("env")
import time

def after_upload(source, target, env):
    print("\n=== Запуск SPIFFS ===\n")
    time.sleep(3)  # Задержка в 3 секунды перед выполнением uploadfs
    env.Execute("platformio run -t uploadfs")

env.AddPostAction("upload", after_upload)
