class A {
    static int x = foo();

    static int foo() {
        if (true) {
            throw new RuntimeException("init failed");
        }
        return 10;
    }
}

public class MainInit {
    public static void main(String[] args) {
        try {
            System.out.println(A.x);
        } catch (Throwable t) {
            System.out.println("first access failed: " + t);
        }

        // 第二次访问
        try {
            System.out.println(A.x);
        } catch (Throwable t) {
            System.out.println("second access failed: " + t);
        }
    }
}
