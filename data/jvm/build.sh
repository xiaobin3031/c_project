rm -R -f *.class
exec_javac=javac
# 判断$jdk17存在
if [ -e "$jdk17" ]; then
    exec_javac="$jdk17/bin/javac"
fi
javac --release 17 -g:none -parameters Main.java MainInit.java ClinitTest.java