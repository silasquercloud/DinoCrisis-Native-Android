plugins { id("com.android.application") version "8.7.3" apply false }

allprojects {
    tasks.withType<Wrapper>().configureEach {
        gradleVersion = "9.7"
    }
}