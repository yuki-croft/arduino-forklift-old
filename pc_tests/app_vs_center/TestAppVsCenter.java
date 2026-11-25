import com.forklift.controller.logic.*;
import java.nio.file.*;
import java.util.*;

/** Feeds text produced by the real final_center_board firmware (PC harness) through the app's logic. */
public class TestAppVsCenter {
    static int fails = 0;

    static void expect(String name, boolean ok) {
        System.out.println((ok ? "PASS  " : "FAIL  ") + name);
        if (!ok) fails++;
    }

    public static void main(String[] args) throws Exception {
        byte[] data = Files.readAllBytes(Paths.get(args[0]));
        LineAssembler assembler = new LineAssembler();
        List<String> lines = new ArrayList<>();
        for (int i = 0; i < data.length; i += 3) {            // uneven 3-byte chunks, like Bluetooth packets
            int n = Math.min(3, data.length - i);
            lines.addAll(assembler.feed(Arrays.copyOfRange(data, i, i + n), n));
        }
        int unknown = 0;
        for (String l : lines) if (CenterMessage.parse(l).type == CenterMessage.Type.UNKNOWN) unknown++;
        expect("20 lines received", lines.size() == 20);
        expect("no line parsed as UNKNOWN", unknown == 0);

        ControllerState s = new ControllerState();
        apply(s, lines, 0, 4);
        expect("job 3>B running, 3 and B highlighted",
               s.getJobText().equals("3>B") && s.getSelectedLoad() == '3' && s.getSelectedUnload() == 'B');
        apply(s, lines, 4, 13);
        expect("after ST DONE: job and highlights cleared", !s.isJobActive() && s.getSelectedLoad() == 0 && s.getSelectedUnload() == 0);
        apply(s, lines, 13, 15);
        expect("EMPTY 3 / BUSY B: reason shown, nothing highlighted", s.getStatusText().equals("BUSY B") && s.getSelectedLoad() == 0);
        apply(s, lines, 15, 19);
        expect("emergency shown during job 1>A", s.isEmergency() && s.getJobText().equals("1>A"));
        apply(s, lines, 19, 20);
        expect("arrival ST WAIT clears emergency and job", !s.isEmergency() && !s.isJobActive());
        System.out.println("fails=" + fails);
        System.exit(fails == 0 ? 0 : 1);
    }

    static void apply(ControllerState s, List<String> lines, int from, int to) {
        for (String l : lines.subList(from, to)) s.apply(CenterMessage.parse(l));
    }
}
