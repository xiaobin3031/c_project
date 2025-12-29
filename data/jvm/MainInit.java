class A {
    static int x = foo();
    static int foo() {
        return 10;
    }
}

public class MainInit {
    public static void main(String[] args) {
        System.out.println(A.x);
    }
}