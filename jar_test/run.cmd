::@powershell -Command "start-process java -verb runas -argument '-Xmx5G -Xms5G -XX:+UseLargePages -jar AllocTest.jar'"
::@java -Xmx5G -Xms5G -XX:+UseLargePages -jar AllocTest.jar
::@windbg -c "$$<%~dp0dbg.txt" java -Xmx5G -Xms5G -XX:+UseLargePages -jar AllocTest.jar
::@cdb -c "$$<%~dp0dbg.txt" -G java -Xmx5G -Xms5G -XX:+UseLargePages -jar AllocTest.jar delay=2
::@java -Xmx5G -Xms5G -XX:+UseLargePages -jar AllocTest.jar
@java -Xmx3G -Xms3G -XX:+UseLargePages -jar AllocTest.jar