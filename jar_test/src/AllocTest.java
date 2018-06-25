import java.util.LinkedList;
import java.util.concurrent.locks.LockSupport;

public class AllocTest {

    public static void main(String[] args) {
        int size = 1024*1024;
        int cnt = 1;
        long delay = 0;
        for(String arg:args) {
            String[] pair = arg.split("=");
            switch (pair[0]) {
            case "size": size = Integer.parseInt(pair[1]); break;
            case "cnt": cnt = Integer.parseInt(pair[1]); break;
            case "delay": delay = Long.parseLong(pair[1]); break;
            default: System.out.println("Unknown parameter "+arg);
            }
        }
        LinkedList<byte[]> data = new LinkedList<>();
        for(int i=0;i<cnt;i++) {
            data.add(new byte[size]);
        }
        if(delay == 0) {
            LockSupport.park();
        } else {
            LockSupport.parkNanos(delay * 1000000);
        }
    }
}