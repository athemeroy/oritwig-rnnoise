package org.oritwig.audio;

/**
 * Offline, stateful RNNoise binding for signed 16-bit, 48 kHz mono PCM.
 * One instance per recording. Calls on an instance are serialized; use
 * try-with-resources to release the native state. This is not a resampler.
 */
public final class RnnoiseDenoiser implements AutoCloseable {
    public static final int SAMPLE_RATE = 48000;
    public static final int FRAME_SAMPLES = 480;
    static { System.loadLibrary("oritwig_rnnoise_jni"); }
    private long handle;

    public RnnoiseDenoiser() { handle = nativeCreate(); }

    /** Denoises a complete frame in place, returning the model's VAD probability. */
    public synchronized float processFrame(short[] samples) {
        requireOpen();
        if (samples == null) throw new NullPointerException("samples");
        if (samples.length != FRAME_SAMPLES)
            throw new IllegalArgumentException("Exactly 480 samples are required");
        return nativeProcessFrame(handle, samples, 0);
    }

    /** Denoises complete consecutive frames in place; returns one VAD per frame. */
    public synchronized float[] process(short[] samples) {
        requireOpen();
        if (samples == null) throw new NullPointerException("samples");
        if (samples.length == 0 || samples.length % FRAME_SAMPLES != 0)
            throw new IllegalArgumentException("A positive multiple of 480 samples is required");
        float[] vad = new float[samples.length / FRAME_SAMPLES];
        for (int i = 0; i < vad.length; i++)
            vad[i] = nativeProcessFrame(handle, samples, i * FRAME_SAMPLES);
        return vad;
    }

    /** Starts a fresh independent stream; does not flush the previous stream. */
    public synchronized void reset() { requireOpen(); nativeReset(handle); }

    /** Idempotent. Further processing/reset calls throw IllegalStateException. */
    @Override public synchronized void close() {
        if (handle != 0) { nativeDestroy(handle); handle = 0; }
    }

    private void requireOpen() {
        if (handle == 0) throw new IllegalStateException("Denoiser is closed");
    }
    private static native long nativeCreate();
    private static native void nativeDestroy(long handle);
    private static native void nativeReset(long handle);
    private static native float nativeProcessFrame(long handle, short[] samples, int offset);
}
