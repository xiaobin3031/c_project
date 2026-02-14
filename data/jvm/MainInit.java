class A {
    static int x = foo();

    static int foo() {
        if (true) {
            throw new RuntimeException();
        }
        return 10;
    }
}

class B {
}

public class MainInit {
    public static void main(String[] args) {

        int[] aa = new int[]{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        System.out.println(aa instanceof int[]);

        // ---------- 触发类初始化 ----------
        try {
            System.out.println(A.x);
        } catch (Throwable t) {
            System.out.println("first access failed");
        }

        // ---------- 第二次访问 ----------
        try {
            System.out.println(A.x);
        } catch (Throwable t) {
            System.out.println("second access failed");
        }

        Object obj = new B();

        // ---------- instanceof ----------
        try {
            boolean r = obj instanceof A;
            System.out.println("instanceof result: ");
            System.out.println(r);
        } catch (Throwable t) {
            System.out.println("instanceof failed");
        }

        // ---------- checkcast ----------
        try {
            A a = (A) obj;
            System.out.println("checkcast success");
        } catch (Throwable t) {
            System.out.println("checkcast failed");
        }
    }
}
