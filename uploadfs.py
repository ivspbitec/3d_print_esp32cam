Import("env")
import time

def after_upload(source, target, env):
    print("\n=== Запуск SPIFFS ===\n")
    time.sleep(1)  # Задержка в 3 секунды перед выполнением uploadfs
    env.Execute("")

env.AddPostAction("upload", after_upload)
