// Applied into :app's own build script via apply(from = ...) - see the
// generic per-module loop in android/app/build.gradle.kts, which does this for
// any enabled module shipping this exact file. This is NOT a separate Gradle
// subproject: it executes in :app's own Project context, so dependencies{}
// here behaves as if written directly in :app's build.gradle.kts.
// Configurations are added by string name ("implementation", not the typed
// function) because Gradle generates no type-safe accessors for a script
// applied this way - and for the same reason nothing here can name an AGP
// type, since a script applied this way does not get AGP on its compile
// classpath either.
//
// The module's other two Android needs are covered by generic hooks in :app
// and need no code here: modules/modio/android/java is picked up by its
// sourceSets loop, and modules/modio/android/assets is staged at the APK
// asset root by stageModuleAssets.
//
// Modio.java is annotated @Keep so R8 cannot strip a class that is only ever
// looked up by name from JNI. Without this the annotation would arrive only
// transitively, through Play's asset-delivery artifact.
dependencies {
    "implementation"("androidx.annotation:annotation:1.9.1")
}
