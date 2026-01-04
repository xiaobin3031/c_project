public class ClinitTest {

    // =================================================
    // =============== 1. 基础静态初始化 ===============
    // =================================================

    static int A = initA();

    static {
        System.out.println("ClinitTest <clinit> block 1");
    }

    static int B = initB();

    static {
        System.out.println("ClinitTest <clinit> block 2");
    }

    static int initA() {
        System.out.println("initA");
        return 10;
    }

    static int initB() {
        System.out.println("initB");
        return A + 20;
    }

    // =================================================
    // =============== 2. 父类 / 子类 ==================
    // =================================================

    static class Parent {
        static int P = initP();

        static {
            System.out.println("Parent <clinit>");
        }

        static int initP() {
            System.out.println("Parent.initP");
            return 100;
        }
    }

    static class Child extends Parent {
        static int C = initC();

        static {
            System.out.println("Child <clinit>");
        }

        static int initC() {
            System.out.println("Child.initC");
            return P + 1;
        }
    }

    // =================================================
    // =============== 3. static final ==================
    // =================================================

    static final int CONST = 123;

    // =================================================
    // =============== 4. <clinit> 中 new ==============
    // =================================================

    static Object OBJ = new Object();

    // =================================================
    // =============== 5. 异常 clinit ==================
    // =================================================

    static class BadClinit {

        static {
            System.out.println("BadClinit <clinit> start");
            fail();
        }

        static void fail() {
            throw new RuntimeException("boom");
        }
    }

    // =================================================
    // =============== 6. 循环依赖 =====================
    // =================================================

    static class LoopA {
        static int X = LoopB.Y + 1;
        static {
            System.out.println("LoopA <clinit>");
        }
    }

    static class LoopB {
        static int Y = 1;
        static {
            System.out.println("LoopB <clinit>");
        }
    }

    // =================================================
    // ===================== main ======================
    // =================================================

    public static void main(String[] args) {

        System.out.println("==== main start ====");

        // 1️⃣ 触发 ClinitTest <clinit>
        System.out.println("print A");
        System.out.println(A);

        System.out.println("print B");
        System.out.println(B);

        // 2️⃣ 验证 clinit 只执行一次
        System.out.println("print A again");
        System.out.println(A);

        // 3️⃣ 父类 / 子类初始化顺序
        System.out.println("print Child.C");
        System.out.println(Child.C);

        // 4️⃣ static final
        System.out.println("print CONST");
        System.out.println(CONST);

        // 5️⃣ new 是否触发 clinit
        System.out.println("print OBJ");
        System.out.println(OBJ);

        // 6️⃣ clinit 抛异常
        try {
            System.out.println("try BadClinit");
            BadClinit b = new BadClinit();
            System.out.println(b);
        } catch (Throwable t) {
            System.out.println("catch BadClinit error");
        }

        // 7️⃣ 循环依赖
        System.out.println("print LoopA.X");
        System.out.println(LoopA.X);

        System.out.println("==== main end ====");
    }
}
