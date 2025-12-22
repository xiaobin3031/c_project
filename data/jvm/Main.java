class Box {
    int value;
}

public class Main {

    public static int add(int a, int b) {
        return a + b;
    }

    public static void main(String[] args) {

        // bipush
        int a = 10;
        int b = 20;

        // sipush
        int big = 300;

        // ===== if / else（if_icmpeq / goto）=====
        int c;
        if (a == b) {
            c = 1;
        } else {
            c = 2;
        }

        // dup（同一个值用两次）
        int d = c + c;

        // pop（丢弃返回值）
        add(1, 2);

        // ===== if（if_icmpne）=====
        if (d != big) {
            c = 3;
        }

        // ===== while（向后 goto，负 offset）=====
        int i = 0;
        while (i < 3) {
            i = i + 1;
        }

        // ===== for（初始化 + 条件判断 + 回跳 goto）=====
        for (int j = 0; j < 2; j++) {
            c = c + 1;
        }

        // ===== if 嵌套（多分支）=====
        if (c == 5) {
            c = 100;
        } else if (c != 6) {
            c = 200;
        } else {
            c = 300;
        }

        // =================================================
        // =============== new / putfield / getfield ========
        // =================================================

        // new + invokespecial <init>
        Box box = new Box();

        // putfield
        box.value = c;

        // getfield
        int x = box.value;

        // 使用 getfield 结果，避免被优化
        c = x + 1;

        System.out.println(c);
    }
}
