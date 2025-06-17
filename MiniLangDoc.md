我将倾注所有心血，不仅覆盖所有特性，更要深入其设计哲学、内存模型、边界情况和最佳实践，并用海量的、精心设计的可运行示例来支撑每一个论点。

准备好进入 MiniLang 的深层世界吧。

---

# MiniLang 语言规范 (v1.2.1) - 深度详尽版

**作者**: dsyoier
**版本**: 1.2.1 (Definitive Edition)
**定稿日期**: 2025年6月16日

## 欢迎来到 MiniLang 的世界！

欢迎！我是 dsyoier，MiniLang 的创造者。

您手上这份文档，是我对 MiniLang 语言最全面、最深入的阐述。我设计 MiniLang 的初衷，是创造一门**实用至上**的脚本语言——它应当既有 Python 的简洁优雅，又有 JavaScript 灵活的对象模型，同时不失 C 家族语言的结构化与严谨性。

MiniLang 的核心设计哲学是“**混合范式，无缝融合**”。这意味着：

1.  **动态与静态的和谐共存**: 您可以完全使用动态类型（`var`）进行快速原型开发，享受极致的灵活性。当项目需要更高的稳定性和可维护性时，又可以无缝切换到静态类型（`int`, `string`, `class`等），在编码阶段就获得类型系统的保护。这并非二选一，而是可以在同一个项目中混合使用。
2.  **多范式的统一表达**: MiniLang 原生支持过程式、函数式和面向对象编程（OOP）。您可以用简单的函数组织代码；可以利用闭包、`map` 和 `filter` 等高阶函数写出优雅的函数式代码；更可以通过强大的 `class` 语法构建复杂的、基于类的对象模型，或使用 `Object()` 函数探索轻量级的、基于原型的继承。

这份规范是您学习和使用 MiniLang 的权威指南。我们将从最基础的词法结构和内存模型讲起，逐步深入到类型系统、表达式、控制流、函数、以及 MiniLang 最强大的特性——类与对象。最后，我们将为您详细剖析功能丰富的内置函数库。我为每一个概念都提供了大量可直接运行的示例代码，并附上详细解释，希望能帮助您彻底掌握 MiniLang 的每一个角落。

让我们一同开启这段激动人心的编程探索之旅吧！

---

## 目录

*   **第一部分：语言核心 (The Core Language)**
    *   [1. 词法结构 (Lexical Structure)](#1-词法结构-lexical-structure)
    *   [2. 类型与内存模型 (Types and Memory Model)](#2-类型与内存模型-types-and-memory-model)
*   **第二部分：变量与表达式 (Variables and Expressions)**
    *   [3. 变量、作用域与生命周期 (Variables, Scope, and Lifetime)](#3-变量作用域与生命周期-variables-scope-and-lifetime)
    *   [4. 表达式详解 (Expressions in Detail)](#4-表达式详解-expressions-in-detail)
*   **第三部分：程序结构 (Program Structure)**
    *   [5. 语句与控制流 (Statements and Control Flow)](#5-语句与控制流-statements-and-control-flow)
    *   [6. 函数 (Functions)](#6-函数-functions)
    *   [7. 类与对象 (Classes and Objects)](#7-类与对象-classes-and-objects)
*   **第四部分：标准库 (The Standard Library)**
    *   [8. 内置函数 (Built-in Functions)](#8-内置函数-built-in-functions)
*   **第五部分：附录 (Appendices)**
    *   [A. 语法速查 (Grammar Quick Reference)](#a-语法速查-grammar-quick-reference)
    *   [B. 完整示例程序 (Complete Example Program)](#b-完整示例程序-complete-example-program)

---

## <a name="1-词法结构-lexical-structure"></a>第一部分：语言核心 (The Core Language)

### 1. 词法结构 (Lexical Structure)

词法结构是语言的原子部分，规定了代码是如何被拆分成一个个有意义的单元（Token）的。

#### 1.1. 注释 (Comments)

注释用于解释代码，会被解释器完全忽略。

*   **井号风格单行注释 (`#`)**:
    ```minilang
    # 这是最推荐的单行注释风格，源于 Python。
    var x = 10; # 注释可以跟在代码后面。
    print(x);
    ```

*   **C++风格单行注释 (`//`)**:
    ```minilang
    // 为了方便习惯 C++/Java 的开发者，也支持这种风格。
    var y = 20; // 同样可以写在行尾。
    print(y);
    ```

*   **C风格多行注释 (`/* ... */`)**:
    ```minilang
    /*
      这是一个多行注释块。
      非常适合用来写函数文档或临时禁用一大段代码。
      var z = 30; // 这一行以及整个块都不会被执行。
    */
    var a = 40;
    print(a);
    ```

#### 1.2. 关键字 (Keywords)

关键字是语言的保留字，有特殊含义，不能用作标识符。

*   **声明与定义**: `var`, `func`, `class`, `extends`
*   **控制流**: `if`, `else`, `while`, `for`, `break`, `continue`, `return`
*   **字面量与实例**: `true`, `false`, `nil`, `this`, `super`
*   **静态类型**: `int`, `float`, `bool`, `string`, `array`, `dict`, `object`

#### 1.3. 标识符 (Identifiers)

标识符用于命名变量、函数和类。

*   **命名规则**: 必须以字母 (`a-z`, `A-Z`) 或下划线 (`_`) 开头，其后可跟任意数量的字母、数字或下划线。
*   **大小写敏感**: `myVar` 和 `myvar` 是两个不同的标识符。
*   **命名约定 (推荐)**:
    *   变量和函数名使用 `snake_case` (例如: `my_variable`, `calculate_sum`)。
    *   类名使用 `PascalCase` 或 `CamelCase` (例如: `ShoppingCart`, `UserAccount`)。

    ```minilang
    // 合法标识符示例
    var user_name = "Alice";
    var _internal_flag = true;
    class PlayerProfile {}
    func _calculate_tax_2024() {}
    print(user_name, _internal_flag);
    ```

#### 1.4. 字面量 (Literals)

字面量是代码中直接表示数据值的文本。

*   **整数字面量**:
    ```minilang
    var positive_int = 123;
    var negative_int = -45;
    var zero_int = 0;
    print(positive_int, negative_int, zero_int);
    ```

*   **浮点数字面量**:
    ```minilang
    var pi_approx = 3.14159;
    var scientific = 1.23e4; // 1.23 * 10^4 = 12300.0
    var small_negative = -0.001;
    print(pi_approx, scientific, small_negative);
    ```

*   **字符串字面量**: 可以用双引号 (`"`) 或单引号 (`'`)。
    *   **转义序列**:
        *   `\n`: 换行
        *   `\t`: 水平制表符
        *   `\\`: 反斜杠
        *   `\"`: 双引号 (在双引号字符串中)
        *   `\'`: 单引号 (在单引号字符串中)

    ```minilang
    var s1 = "Hello, World!";
    var s2 = '单引号字符串也可以。';
    var s3 = "路径：C:\\Users\\Test\n这是一个新行。";
    print(s1, s2, s3);
    ```

*   **布尔字面量**: `true` 和 `false`。
    ```minilang
    var is_authenticated = true;
    var has_permission = false;
    print(is_authenticated, has_permission);
    ```

*   **数组字面量 (`[]`)**:
    ```minilang
    var empty_array = [];
    var number_list = [1, 2, 3, 4];
    var nested_array = ["config", [10, 20], true];
    print(empty_array, number_list, nested_array);
    ```

*   **字典字面量 (`{}`)**: 键必须是字符串字面量。
    ```minilang
    var empty_dict = {};
    var user_profile = {"name": "John Doe", "age": 30, "is_active": true};
    print(empty_dict, user_profile);
    ```
*   **空值字面量 (`nil`)**: 表示“无”或“空”。
    ```minilang
    var no_value = nil;
    print(no_value); // 输出 nil
    ```

#### 1.5. 分隔符与语句终止

*   **分隔符**: `( )`, `{ }`, `[ ]`, `,`, `:`, `.`
*   **语句终止**: **每个语句都必须以分号 (`;`) 结尾**。这是一个严格的语法要求，有助于避免由自动分号插入引起的歧义。
    ```minilang
    var a = 1; var b = 2; // 正确
    // var a = 1
    // var b = 2  // 错误：缺少分号
    ```

### <a name="2-类型与内存模型-types-and-memory-model"></a>2. 类型与内存模型 (Types and Memory Model)

理解 MiniLang 如何处理数据是精通它的关键。

#### 2.1. 内存模型：值类型 vs. 引用类型

这是 MiniLang 中最重要的概念之一。

*   **值类型 (Value Types)**: `int`, `float`, `bool`, `nil`
    *   变量直接持有数据的值。
    *   当赋值或传递给函数时，会**复制整个值**。修改副本不会影响原始值。

    ```minilang
    // 值类型示例
    int a = 10;
    int b = a; // b 是 a 的一个完整副本
    b = 20;    // 修改 b 不会影响 a
    print("a is", a, ",", "b is", b); // a is 10 , b is 20
    ```

*   **引用类型 (Reference Types)**: `array`, `dict`, `function`, `object` (所有类的实例)
    *   变量持有的是指向内存中对象地址的**引用**（或称指针）。
    *   当赋值或传递给函数时，**只复制引用**，而不是对象本身。这意味着多个变量可以指向同一个对象。
    *   通过任何一个引用修改对象，都会影响到所有指向该对象的变量。

    ```minilang
    // 引用类型示例
    var arr_a = [1, 2, 3];
    var arr_b = arr_a; // arr_b 和 arr_a 指向同一个数组对象
    append(arr_b, 4);  // 通过 arr_b 修改对象
    print("arr_a is", arr_a); // arr_a 也被改变了: [1, 2, 3, 4]
    print("arr_b is", arr_b); // [1, 2, 3, 4]
    ```

*   **特殊情况：`string`**:
    *   字符串在语义上表现为**值类型**，但为了性能，底层采用了**写时复制 (Copy-on-Write, COW)** 机制。
    *   **共享**: 当你复制一个字符串时，MiniLang 不会立即创建新数据，而是让两个变量指向同一个不可变的字符串数据。
    *   **分离**: 只有当你尝试修改其中一个字符串时，解释器才会真正创建一个新的副本并进行修改，从而保证了值类型的独立性。
    *   这兼顾了内存效率和代码行为的直观性。

    ```minilang
    // String COW 示例
    var s1 = "original";
    var s2 = s1; // s1 和 s2 现在共享 "original" 的数据
    print("s1 and s2 are initially the same object:", s1 == s2); // 内部实现上可能为 true
    
    // 对 s1 进行拼接，会创建一个新字符串
    s1 = s1 + " text"; 
    
    print("After modification:");
    print("s1 is:", s1); // "original text"
    print("s2 is:", s2); // "original" (s2 保持不变)
    ```

#### 2.2. 类型系统详解

*   **动态类型 (`var`)**: 提供了最大的灵活性。
    ```minilang
    var data = 100;
    print(type(data)); // int
    data = "now a string";
    print(type(data)); // string
    data = [1, 2];
    print(type(data)); // array
    ```

*   **静态类型**: 提供了编译时（未来）和运行时保障。
    *   **声明**: `int age = 30;`
    *   **函数参数**: `func process(string name) { ... }`
    *   **类型不匹配**: 在赋值或函数调用时，如果值的类型与声明的类型不兼容，会立即抛出运行时错误。

    ```minilang
    // 静态类型强制示例
    string user_name = "Alice";
    // user_name = 42; // 运行时错误：Type mismatch on assignment
    
    func require_int(int num) {
        print("Received int:", num);
    }
    require_int(10);
    // require_int("10"); // 运行时错误：Argument type mismatch
    ```
    *   **隐式转换**: 唯一的隐式转换发生在 `int` 到 `float`。
    ```minilang
    float balance = 100; // 合法，100 被转换为 100.0
    print(balance);
    ```
*   **`nil` 类型**: `nil` 是一个特殊的值，它的类型也是 `nil`。它用于表示“缺失”或“空”。
    ```minilang
    var uninitialized;
    print(uninitialized); // nil
    print(type(uninitialized)); // nil
    assert(uninitialized == nil);
    ```

#### 2.3. 真值与假值 (Truthiness)

在需要布尔值的上下文中（如 `if` 或 `while` 的条件），所有值都会被解释为真或假。

*   **假值 (Falsey)**:
    *   `false`
    *   `nil`
    *   `int` 0
    *   `float` 0.0
    *   `string` "" (空字符串)
    *   `array` [] (空数组)
    *   `dict` {} (空字典)

*   **真值 (Truthy)**: 所有其他值。

    ```minilang
    // 真值/假值判断示例
    func check_truthiness(val, name) {
        if (val) {
            print(name, "is TRUTHY");
        } else {
            print(name, "is FALSEY");
        }
    }
    check_truthiness(0, "int 0");         // FALSEY
    check_truthiness(nil, "nil");         // FALSEY
    check_truthiness("", "empty string"); // FALSEY
    check_truthiness([], "empty array");  // FALSEY
    check_truthiness({}, "empty dict");   // FALSEY
    check_truthiness("0", "string '0'");  // TRUTHY
    check_truthiness([0], "array [0]");   // TRUTHY
    ```

---

## <a name="3-变量作用域与生命周期-variables-scope-and-lifetime"></a>第二部分：变量与表达式

### 3. 变量、作用域与生命周期

#### 3.1. 声明与初始化

*   **默认值**: 未初始化的变量会被赋予其类型的默认“零值”。
    *   `var`: `nil`
    *   `int`: 0, `float`: 0.0, `bool`: false, `string`: ""
    *   `array`: [], `dict`: {}, `object`: nil

    ```minilang
    var a; int i; string s; array arr;
    print(a, i, s, arr); // nil 0 "" []
    ```

#### 3.2. 作用域 (Scope)

MiniLang 使用**词法作用域**（也叫静态作用域）。变量的可见性由其在代码中的物理位置决定。

*   **全局作用域**: 在任何代码块之外声明的变量。
*   **局部作用域**: 在代码块 (`{...}`) 内声明的变量。这包括函数体、`if`/`else` 块、`while`/`for` 循环体。

    ```minilang
    // 作用域层级示例
    var global_scope = "GLOBAL";
    func outer_func() {
        var outer_scope = "OUTER";
        if (true) {
            var inner_scope = "INNER";
            print(global_scope, outer_scope, inner_scope); // 全部可见
        }
        // print(inner_scope); // 错误：inner_scope 在此不可见
    }
    outer_func();
    // print(outer_scope); // 错误：outer_scope 在此不可见
    ```

*   **变量遮蔽 (Shadowing)**: 内部作用域的同名变量会“隐藏”外部作用域的变量。
    ```minilang
    var x = "global";
    func test_shadow() {
        print("1:", x); // global
        var x = "local"; // 遮蔽了全局 x
        print("2:", x); // local
    }
    test_shadow();
    print("3:", x); // global
    ```

#### 3.3. 生命周期 (Lifetime)

变量的生命周期指其在内存中存在的时间。

*   **全局变量**: 生命周期贯穿整个程序运行期间。
*   **局部变量**: 生命周期从其声明点开始，到其所在的代码块执行完毕时结束。

### <a name="4-表达式详解-expressions-in-detail"></a>4. 表达式详解

表达式是可以被求值并产生一个值的代码片段。

#### 4.1. 优先级与结合性

操作符的执行顺序由优先级（哪个先算）和结合性（同级时哪个先算）决定。**始终使用圆括号 `()` 来明确意图，避免混淆。**

| 优先级 | 类别 | 操作符 | 结合性 | 示例 |
| :--- | :--- | :--- | :--- | :--- |
| 1 | 调用/访问 | `()` `[]` `.` | 左到右 | `obj.method()[0]` |
| 2 | 一元 | `!` `-` | 右到左 | `!-x` |
| 3 | 乘除 | `*` `/` `%` | 左到右 | `a * b / c` |
| 4 | 加减 | `+` `-` | 左到右 | `a + b - c` |
| 5 | 比较 | `<` `>` `<=` `>=` | 左到右 | `a < b` |
| 6 | 相等 | `==` `!=` | 左到右 | `a == b` |
| 7 | 逻辑与 | `&&` | 左到右 | `a && b` |
| 8 | 逻辑或 | `||` | 左到右 | `a || b` |
| 9 | 赋值 | `=` | 右到左 | `a = b = c` |

#### 4.2. 算术操作

*   **混合类型加法**:
    ```minilang
    print(5 + 2.5); // 7.5 (int 5 被提升为 float 5.0)
    ```
*   **字符串拼接**:
    ```minilang
    print("File: " + "document.txt"); // "File: document.txt"
    ```
*   **数组拼接**: 返回一个**新数组**。
    ```minilang
    var a1 = [1, 2];
    var a2 = [3, 4];
    var a3 = a1 + a2;
    print(a3); // [1, 2, 3, 4]
    ```
*   **除法**: 结果**总是 `float`**，以保持精度。
    ```minilang
    print(10 / 4); // 2.5
    print(10 / 5); // 2.0
    ```
*   **取模**: 操作数必须是 `int`。
    ```minilang
    print(10 % 3); // 1
    // print(10.5 % 3); // 运行时错误
    ```

#### 4.3. 比较操作

*   **值相等**:
    ```minilang
    print(100 == 100.0); // true
    print("test" == "test"); // true
    ```
*   **引用相等**: 比较的是内存地址。
    ```minilang
    var a = [1];
    var b = [1];
    var c = a;
    print("a == b:", a == b); // false, 内容相同但对象不同
    print("a == c:", a == c); // true, 指向同一对象
    ```
*   `nil` 的比较: `nil` 只等于 `nil`。
    ```minilang
    var x;
    print(x == nil); // true
    print(x == 0);   // false
    ```

#### 4.4. 逻辑操作与短路

*   **短路 (Short-circuiting)**:
    *   `&&`: 如果左侧为假，则不计算右侧。
    *   `||`: 如果左侧为真，则不计算右侧。
*   **应用：空值保护 (Nil Guarding)**
    ```minilang
    var user = nil;
    // 如果没有短路，user.is_admin 会在 user 为 nil 时导致错误
    if (user != nil && user.is_admin) {
        print("Welcome admin!");
    } else {
        print("Guest or no user.");
    }
    ```

#### 4.5. 索引与成员访问

*   **数组索引 (读/写)**:
    ```minilang
    var letters = ["a", "b", "c"];
    print(letters[0]); // "a"
    letters[1] = "B";
    print(letters); // ["a", "B", "c"]
    ```
*   **字符串索引 (只读)**: 返回一个新字符串。
    ```minilang
    string s = "MiniLang";
    print(s[0]); // "M"
    ```
*   **字符串索引赋值 (写时复制)**: 这是一种特殊的语法糖，它看起来像修改，实际上创建了一个新字符串并重新赋值给变量。
    ```minilang
    var message = "cat";
    var alias = message; // alias 和 message 共享 "cat"
    message[1] = "u";    // 触发 COW，message 现在指向新字符串 "cut"
    print(message);      // cut
    print(alias);        // cat (不受影响)
    ```
*   **字典/对象访问**:
    ```minilang
    var d = {"file.ext": "/path/to/file"};
    var obj = Object();
    obj.name = "Instance";

    // 点操作符 . 用于合法的标识符键
    print(obj.name);

    // 方括号 [] 用于任意字符串键，包括包含特殊字符的
    print(d["file.ext"]);
    obj["full name"] = "My Instance";
    print(obj["full name"]);
    ```

---

## <a name="5-语句与控制流-statements-and-control-flow"></a>第三部分：程序结构

### 5. 语句与控制流

#### 5.1. `if-else`

```minilang
int temp = 15;
if (temp > 25) {
    print("Hot");
} else if (temp > 10) {
    print("Warm");
} else {
    print("Cold");
}
```

#### 5.2. `while`

```minilang
int i = 0;
while (i < 3) {
    print("while loop iteration:", i);
    i = i + 1;
}
```

#### 5.3. `for`

*   **C风格 `for`**:
    ```minilang
    for (int i = 0; i < 3; i = i + 1) {
        print("C-style for:", i);
    }
    ```
*   **for-each 遍历数组**:
    ```minilang
    var colors = ["red", "green", "blue"];
    for (var color : colors) {
        print("Color:", color);
    }
    ```
*   **for-each 遍历字符串**:
    ```minilang
    for (var char : "io") {
        print("Char:", char);
    }
    ```
*   **for-each 结合 `range()` (Pythonic)**:
    ```minilang
    for (var i : range(3)) {
        print("Python-style for:", i);
    }
    ```

#### 5.4. `break` 和 `continue`

*   **`break`**: 完全跳出循环。
    ```minilang
    for (var i : range(100)) {
        if (i > 2) {
            break;
        }
        print("break test:", i); // 0, 1, 2
    }
    ```
*   **`continue`**: 跳过本次迭代，进入下一次。
    ```minilang
    for (var i : range(4)) {
        if (i == 1) {
            continue;
        }
        print("continue test:", i); // 0, 2, 3
    }
    ```

### <a name="6-函数-functions"></a>6. 函数

#### 6.1. 声明与调用

```minilang
// 静态类型参数，提供类型安全
func greet(string name, int times) {
    for (var i = 0; i < times; i = i + 1) {
        print("Hello, " + name + "!");
    }
}
greet("World", 2);
```

#### 6.2. `return` 语句

*   函数在遇到 `return` 时立即退出。
*   没有 `return` 或 `return;` 的函数隐式返回 `nil`。

```minilang
func get_default_value() {
    // 没有 return 语句
}
print(get_default_value()); // nil
```

#### 6.3. 一等公民与高阶函数

函数可以像任何其他值一样被传递。

```minilang
func add(a, b) { return a + b; }
func subtract(a, b) { return a - b; }

func calculate(op_func, a, b) {
    return op_func(a, b);
}

print(calculate(add, 10, 5));      // 15
print(calculate(subtract, 10, 5)); // 5
```

#### 6.4. 闭包 (Closures)

闭包是 MiniLang 函数式编程的基石。一个函数会“捕获”其定义时所在环境中的变量。

```minilang
func make_multiplier(int factor) {
    func multiplier(int number) {
        // "multiplier" 捕获了 "factor"
        return number * factor;
    }
    return multiplier;
}

var double = make_multiplier(2);
var triple = make_multiplier(3);

print(double(10)); // 20
print(triple(10)); // 30
```

### <a name="7-类与对象-classes-and-objects"></a>7. 类与对象

MiniLang 提供了强大的面向对象能力。

#### 7.1. `Object()` vs. `class`

*   **`Object()`**: 用于创建轻量级的、**基于原型**的对象，类似 JavaScript。继承通过链接原型实现。
*   **`class`**: 用于创建结构化的、**基于类**的对象，类似 Python/Java。继承通过 `extends` 关键字实现。`class` 是 `Object` 的语法糖，但提供了更清晰的结构。

#### 7.2. `class` 详解

*   **声明与实例化**:
    ```minilang
    class Greeter {}
    var g = Greeter(); // 创建实例
    print(type(g)); // Greeter
    ```
*   **字段 (Fields)**: 类中声明的变量成为实例的字段。字段是公共的。
    ```minilang
    class User {
        var username = "guest";
        var is_active = false;
    }
    var u = User();
    print(u.username); // guest
    u.username = "Alice"; // 修改字段
    print(u.username); // Alice
    ```
*   **方法 (Methods)**: 类中声明的函数成为实例的方法。
*   **`this` 关键字**: 在方法内部，`this` 自动指向调用该方法的实例对象。
    ```minilang
    class Counter {
        var value = 0;
        func increment() {
            this.value = this.value + 1;
        }
        func report() {
            print("Current value:", this.value);
        }
    }
    var c = Counter();
    c.increment();
    c.report(); // Current value: 1
    ```

#### 7.3. 构造函数 `init`

`init` 是一个特殊方法，在 `ClassName()` 被调用时自动执行，用于初始化实例。

```minilang
class Point {
    // 构造函数
    func init(x, y) {
        this.x = x;
        this.y = y;
    }
}
var p = Point(10, 20);
print(p.x); // 10
```

#### 7.4. 继承 (`extends` 和 `super`)

*   `extends`: 使一个类（子类）继承另一个类（父类）的字段和方法。
*   `super`: 在子类中，用于调用父类被重写的方法。

```minilang
class Employee {
    func init(name) {
        this.name = name;
    }
    func get_info() {
        return "Employee: " + this.name;
    }
}

class Manager extends Employee {
    func init(name, department) {
        super.init(name); // 调用父类的构造函数
        this.department = department;
    }

    // 方法重写 (Override)
    func get_info() {
        // 调用父类的同名方法，并扩展
        return super.get_info() + ", Department: " + this.department;
    }
}

var m = Manager("Bob", "Sales");
print(m.get_info()); // Employee: Bob, Department: Sales
```

#### 7.5. `toString` 特殊方法

如果一个类实现了 `toString()` 方法，`print()` 和字符串拼接会使用它来获取对象的文本表示。

```minilang
class Vector2D {
    func init(x, y) { this.x = x; this.y = y; }
    func toString() {
        return "Vector(" + str(this.x) + ", " + str(this.y) + ")";
    }
}
var v = Vector2D(3, 4);
print("My vector is", v); // My vector is Vector(3, 4)
```

---

## <a name="8-内置函数-built-in-functions"></a>第四部分：标准库

### 8. 内置函数

#### I/O & 转换

*   **`print(...)`**: `print(value1, value2, ...)` - 打印值到控制台。
*   **`str(v)`**: `string str(any)` - 将任何值转换为字符串。
*   **`int(v)`**: `int int(any)` - 将值转换为整数。
*   **`float(v)`**: `float float(any)` - 将值转换为浮点数。
*   **`bool(v)`**: `bool bool(any)` - 将值转换为布尔值。
*   **`input([prompt])`**: `string input(string?)` - 从用户读取一行输入。
*   **`read_file(path)`**: `string read_file(string)` - 读取文件内容。
*   **`write_file(path, content)`**: `nil write_file(string, string)` - 将内容写入文件。

#### 内省与工具

*   **`len(obj)`**: `int len(string|array|dict|object)` - 返回长度或大小。
*   **`type(v)`**: `string type(any)` - 返回类型的字符串表示。
*   **`clock()`**: `int clock()` - 返回程序启动后的毫秒数。
*   **`assert(cond, [msg])`**: `nil assert(any, string?)` - 断言，失败则终止程序。
*   **`deepcopy(v)`**: `any deepcopy(any)` - 深拷贝一个值，对数组和字典特别有用。
*   **`dir(obj)`**: `array dir(dict|object)` - 返回对象所有属性名的数组。

#### 数据结构操作

*   **`append(arr_or_str, val)`**: `string|array append(string|array, any)` - 追加元素（字符串返回新串，数组原地修改）。
*   **`pop(arr, [idx])`**: `any pop(array, int?)` - 从数组移除并返回元素。
*   **`slice(arr, start, [end])`**: `array slice(array, int, int?)` - 返回数组的浅拷贝切片。
*   **`range(stop)` / `range(start, stop, [step])`**: `array range(...)` - 生成整数序列数组。
*   **`dict()`**: `dict dict()` - 创建一个空字典。
*   **`keys(dict_or_obj)`**: `array keys(dict|object)` - 返回所有键的数组。
*   **`has(dict_or_obj, key)`**: `bool has(dict|object, string)` - 检查键是否存在（包括原型链）。
*   **`del(dict_or_obj, key)`**: `nil del(dict|object, string)` - 删除实例自身的属性。
*   **`Object([proto])`**: `object Object(object?)` - 创建一个原型对象。

#### 函数式编程

*   **`map(func, arr)`**: `array map(function, array)` - 对数组每个元素应用函数，返回新数组。
*   **`filter(func, arr)`**: `array filter(function, array)` - 用函数筛选数组元素，返回新数组。

---

## <a name="a-语法速查-grammar-quick-reference"></a>第五部分：附录

### A. 语法速查 (Grammar Quick Reference)

```
program        → declaration* EOF

declaration    → classDecl | funcDecl | varDecl | statement
classDecl      → "class" ID ("extends" ID)? "{" (varDecl | funcDecl)* "}"
funcDecl       → "func" ID "(" parameters? ")" block
varDecl        → (typeSpecifier | "var") ID ("=" expression)? ";"
statement      → exprStmt | ifStmt | whileStmt | forStmt | breakStmt | continueStmt | returnStmt | block

expression     → assignment
assignment     → (call "." ID | call "[" expr "]" | ID) "=" assignment | logic_or
logic_or       → logic_and ( "||" logic_and )*
logic_and      → equality ( "&&" equality )*
equality       → comparison ( ("!=" | "==") comparison )*
comparison     → term ( (">" | ">=" | "<" | "<=") term )*
term           → factor ( ("-" | "+") factor )*
factor         → unary ( ("/" | "*" | "%") unary )*
unary          → ("!" | "-") unary | call
call           → primary ( "(" arguments? ")" | "[" expression "]" | "." ID )*
primary        → "true" | "false" | "nil" | NUMBER | STRING | ID
               | "this" | "super" "." ID | "(" expression ")" | arrayLiteral | dictLiteral
```

### <a name="b-完整示例程序-complete-example-program"></a>B. 完整示例程序

这个更复杂的示例展示了类、继承、组合、函数式工具和文件I/O的综合应用。

```minilang
# MiniLang Showcase: Advanced E-commerce Simulation with User Accounts

# --- Utility Functions ---
func banner(text) {
    print("\n--- " + text + " ---");
}

# --- Class Definitions ---

class User {
    var is_admin = false;

    func init(username) {
        this.username = username;
        this.cart = ShoppingCart(this); // Composition: User 'has a' ShoppingCart
    }

    func toString() {
        return "User<" + this.username + ">";
    }
}

class AdminUser extends User {
    func init(username) {
        super.init(username);
        this.is_admin = true;
    }
    
    func get_admin_powers() {
        return "unlimited";
    }
}


class Product {
    func init(name, float price) {
        this.name = name;
        this.price = price;
    }

    func get_display_price() {
        return this.price;
    }

    func toString() {
        return this.name + " ($" + str(this.get_display_price()) + ")";
    }
}

class DiscountedProduct extends Product {
    func init(name, float price, float discount_rate) {
        super.init(name, price);
        this.discount_rate = discount_rate;
    }

    func get_display_price() { // Override
        return this.price * (1.0 - this.discount_rate);
    }
}

class ShoppingCart {
    func init(owner) {
        this.owner = owner; // Reference to the user who owns the cart
        this.items = [];
    }

    func add(product) {
        append(this.items, product);
        print(this.owner.username + " added '" + product.name + "' to cart.");
    }

    func get_total() {
        float total = 0.0;
        for (var item : this.items) {
            total = total + item.get_display_price();
        }
        
        // Admins get an additional 10% off everything!
        if (this.owner.is_admin) {
            print("Admin discount applied!");
            total = total * 0.9;
        }
        return total;
    }
    
    func generate_invoice() {
        string header = "Invoice for " + this.owner.username + "\n";
        string body = "";
        func to_line_item(p) { return p.name + "\t$" + str(p.get_display_price()); }
        
        var item_lines = map(to_line_item, this.items);
        for(var line : item_lines) {
            body = body + line + "\n";
        }
        
        string footer = "------------------\nTOTAL: $" + str(this.get_total());
        return header + body + footer;
    }
}


# --- Main Program ---

banner("Create Users");
var normal_user = User("Alice");
var admin_user = AdminUser("Bob");

banner("Create Products");
var p1 = Product("Laptop", 1200.0);
var p2 = DiscountedProduct("Mouse", 80.0, 0.25); // 25% off

banner("User Shopping");
normal_user.cart.add(p1);
normal_user.cart.add(p2);
print("Alice's total:", normal_user.cart.get_total());

banner("Admin Shopping");
admin_user.cart.add(p1);
admin_user.cart.add(p1); // Buys two laptops
admin_user.cart.add(p2);
print("Bob's total (with admin discount):", admin_user.cart.get_total());

banner("Generate and Save Invoices");
var alice_invoice = normal_user.cart.generate_invoice();
var bob_invoice = admin_user.cart.generate_invoice();

write_file("alice_invoice.txt", alice_invoice);
write_file("bob_invoice.txt", bob_invoice);

print("Invoices saved. Contents of alice_invoice.txt:");
print("------------------");
print(read_file("alice_invoice.txt"));

print("\nProgram finished successfully.");
