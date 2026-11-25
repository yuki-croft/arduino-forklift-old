junit_shim/ is a tiny stand-in for the JUnit 4 API (@Test, Assert) so the SAME test sources under
app/src/test can be compiled and run with plain javac/java on a PC without Gradle.
It is NOT JUnit. In Android Studio the real JUnit (testImplementation) is used instead.
