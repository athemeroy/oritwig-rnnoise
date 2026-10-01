import java.util.Arrays;
import java.util.concurrent.atomic.AtomicReference;
import org.oritwig.audio.RnnoiseDenoiser;

public final class RnnoiseDenoiserTest {
    private RnnoiseDenoiserTest() {}
    private static void check(boolean value, String detail) {
        if (!value) throw new AssertionError(detail);
    }
    private static void expect(Class<? extends Throwable> type, Runnable action) {
        try { action.run(); }
        catch (Throwable error) {
            if (type.isInstance(error)) return;
            throw new AssertionError("Wrong exception", error);
        }
        throw new AssertionError("Missing " + type.getName());
    }
    private static short[] fixture() {
        short[] result = new short[480 * 20];
        int random = 12345;
        for (int i = 0; i < result.length; i++) {
            random = random * 1664525 + 1013904223;
            result[i] = (short)(9000 * Math.sin(i * Math.PI * 2 * 137 / 48000)
                    + ((random >>> 20) - 2048));
        }
        return result;
    }
    private static void contract() {
        try (RnnoiseDenoiser a = new RnnoiseDenoiser(); RnnoiseDenoiser b = new RnnoiseDenoiser()) {
            short[] original = fixture(), batch = original.clone(), chunked = new short[original.length];
            float[] vad = a.process(batch);
            for (int f = 0; f < vad.length; f++) {
                short[] frame = Arrays.copyOfRange(original, f * 480, (f + 1) * 480);
                float value = b.processFrame(frame);
                check(Float.isFinite(value) && value >= 0 && value <= 1, "VAD range");
                check(value == vad[f], "VAD chunk equivalence");
                System.arraycopy(frame, 0, chunked, f * 480, 480);
            }
            check(Arrays.equals(batch, chunked), "PCM chunk equivalence");
            check(!Arrays.equals(batch, original), "Engine must not pass through");
            expect(IllegalArgumentException.class, () -> a.process(new short[479]));
            expect(IllegalArgumentException.class, () -> a.process(new short[0]));
            expect(IllegalArgumentException.class, () -> a.processFrame(new short[960]));
            expect(NullPointerException.class, () -> a.process(null));
            a.reset();
            short[] repeated = original.clone();
            check(Arrays.equals(vad, a.process(repeated)), "Reset VAD equivalence");
            check(Arrays.equals(batch, repeated), "Reset PCM equivalence");
            a.reset();
            short[] silence = new short[480];
            float value = a.processFrame(silence);
            check(Float.isFinite(value), "Silence finite");
            check(Arrays.equals(silence, new short[480]), "Silence stable");
        }
        RnnoiseDenoiser closed = new RnnoiseDenoiser();
        closed.close(); closed.close();
        expect(IllegalStateException.class, () -> closed.processFrame(new short[480]));
        expect(IllegalStateException.class, closed::reset);
    }
    public static void main(String[] args) throws InterruptedException {
        AtomicReference<Throwable> failure = new AtomicReference<>();
        Thread[] threads = new Thread[4];
        for (int i = 0; i < threads.length; i++) {
            threads[i] = new Thread(() -> {
                try { for (int iteration = 0; iteration < 8; iteration++) contract(); }
                catch (Throwable error) { failure.compareAndSet(null, error); }
            });
            threads[i].start();
        }
        for (Thread thread : threads) thread.join();
        if (failure.get() != null) throw new AssertionError("JNI test failed", failure.get());
        System.out.println("PASS: real JNI, PCM processing, VAD, chunking, reset, close, validation, concurrent independent streams");
    }
}
