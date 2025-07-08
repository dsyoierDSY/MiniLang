# MiniLang 语言官方权威指南 (v1.3.0 - 新手友好终极版)

**作者**: dsyoier
**版本**: 1.3.0 (The Definitive Guide for Everyone)
**定稿日期**: 2025年6月17日

## 欢迎来到 MiniLang 的世界！

你好！我是 dsyoier，MiniLang 的创造者。

无论你是一位经验丰富的程序员，还是今天才第一次决定尝试编程，这份指南都为你而写。我创造 MiniLang 的目标是：**打造一门既强大又友好的语言**。它应该像 Python 一样易于阅读，像 JavaScript 一样灵活，同时具备成为大型项目基石的稳固结构。

**如果你是编程新手...**
别担心！我们将从零开始。我会用生活中的例子帮你理解“变量”、“函数”、“循环”这些概念。你将从写出第一行 `print("Hello, World!");` 开始，一步步建立信心，最终能够独立编写出有趣的小程序。MiniLang 就是你进入奇妙编程世界的最佳起点。

**如果你是经验丰富的开发者...**
你会发现 MiniLang 许多熟悉而又精巧的设计。它的混合类型系统、双范式对象模型（类式与原型式并存）、安全的模块化机制以及强大的内置函数库，将为你提供一个高效、愉悦的开发环境。你可以直接跳到你感兴趣的章节，例如**第九章：模块化** 或 **第十一章：高级主题**。

这份指南是学习 MiniLang 的唯一官方资料。它包含了你需要知道的一切。让我们一起，开启这段激动人心的旅程吧！

---

## 目录

*   **第一部分：踏上征程 (Getting Started)**
    *   [1. 你的第一个程序：Hello, World!](#1-你的第一个程序)
    *   [2. 存储信息：变量与数据类型](#2-存储信息-变量与数据类型)
    *   [3. 基本运算：让程序会计算](#3-基本运算)
*   **第二部分：控制程序的脉搏 (Controlling the Flow)**
    *   [4. 做出选择：`if-else` 语句](#4-做出选择-if-else-语句)
    *   [5. 重复工作：`while` 与 `for` 循环](#5-重复工作-循环)
*   **第三部分：构建坚实的程序大厦 (Building Blocks)**
    *   [6. 代码的“食谱”：函数](#6-代码的食谱-函数)
    *   [7. 整理数据：数组与字典](#7-整理数据-数组与字典)
    *   [8. 程序的“蓝图”：类与对象入门](#8-程序的蓝图-类与对象)
*   **第四部分：成为高级工匠 (Advanced Craftsmanship)**
    *   [9. 组织代码：`import` 与 `include` 的艺术](#9-组织代码-模块化)
    *   [10. 优雅地处理错误：`try-catch` 机制](#10-优雅地处理错误)
    *   [11. 深度探索：高级主题与设计哲学](#11-深度探索-高级主题)
*   **第五部分：实用工具箱 (The Reference Toolkit)**
    *   [12. 内置函数大全](#12-内置函数大全)
    *   [13. 语法速查表](#13-语法速查表)

---

## <a name="1-你的第一个程序"></a>第一部分：踏上征程 (Getting Started)

### 1. 你的第一个程序：Hello, World!

在编程世界里，学习任何新语言的第一步，传统上都是让电脑向世界问好。这很简单，但意义非凡——它证明了你和电脑已经成功建立了沟通！

在 MiniLang 中，我们这样写：

```minilang
// file: hello.mylang
print("Hello, World!");
```

将这段代码保存到一个名为 `hello.mylang` 的文件中，然后运行它，你会在屏幕上看到：

```
Hello, World!
```

恭喜你！你已经是一个 MiniLang 程序员了！现在，让我们分解一下这行神奇的代码：

*   **`print`**: 这是一个**内置函数**。你可以把它想象成电脑的一个技能，这个技能的作用就是把它括号里的东西显示在屏幕上。
*   **`(...)`**: 圆括号是用来“调用”函数的，我们把想让函数处理的东西（称为“参数”）放在里面。
*   **`"Hello, World!"`**: 这是一段**字符串**，也就是普通的文本。在 MiniLang 中，文本需要用双引号 `"` 或单引号 `'` 包起来，这样电脑才知道它是一段文字，而不是命令。
*   **`;`**: **分号是 MiniLang 中一个句子的句号**。它告诉解释器：“这条指令到这里就结束了。” 每一条完整的指令后面都必须跟一个分号。

### <a name="2-存储信息-变量与数据类型"></a>2. 存储信息：变量与数据类型

程序不仅要执行指令，还需要临时存储和处理信息。我们使用**变量**来做这件事。

#### 2.1. 什么是变量？

想象一个带标签的盒子。你可以往里面放东西，并且随时可以通过标签找到这个盒子，看看里面是什么，或者换成别的东西。

在 MiniLang 中，这个“带标签的盒子”就是**变量**。

```minilang
var user_name = "Alice"; // 创建一个叫 user_name 的盒子，放入文本 "Alice"
var user_age = 30;       // 创建一个叫 user_age 的盒子，放入数字 30

print(user_name);        // 查看 user_name 盒子的内容并打印
print(user_age);         // 查看 user_age 盒子的内容并打印

user_age = 31;           // 更新 user_age 盒子的内容
print("Happy Birthday! Age is now:", user_age);
```
**Output:**
```
Alice
30
Happy Birthday! Age is now: 31
```
*   `var`: 这是“我需要一个新盒子”的信号，即声明一个新变量。
*   `=`: 这不是数学上的“等于”，而是“**赋值**”操作，意思是“把右边的东西，放进左边的盒子里”。

#### 2.2. 信息的种类：数据类型

你不能把液体和书本放在同一个盒子里，对吧？电脑也一样，它需要知道存储的信息是什么类型的。

**动态类型 (`var`)**
当你使用 `var` 时，你告诉 MiniLang：“创建一个通用的盒子，它可以放任何类型的东西。” 这很灵活，非常适合快速编程。

```minilang
var stuff = 100;       // 现在 stuff 盒子里是数字
print(stuff);
stuff = "一百";      // 现在盒子里换成了文本
print(stuff);
```

**静态类型 (更具体的盒子)**
有时，为了让程序更稳定、更清晰，我们想指定一个盒子**只能**放特定类型的东西。

*   `int`: 整数 (e.g., `-5`, `0`, `42`)
*   `float`: 小数 (浮点数) (e.g., `-0.5`, `3.14`)
*   `string`: 文本 (字符串)
*   `bool`: 布尔值，表示“真”或“假”，只有两个值：`true` 和 `false`。

```minilang
string player_name = "Bob";
int score = 95;
bool is_online = true;

// player_name = 123; // 这会报错！因为 player_name 盒子被指定只能放 string。
```

#### 2.3. 特殊值 `nil`
有时，一个盒子可能是空的。在 MiniLang 中，我们用 `nil` 来表示“无”或“空值”。一个变量在声明后如果没有被赋值，它的默认值就是 `nil`。

```minilang
var data;
print(data); // Output: nil
```

### <a name="3-基本运算"></a>3. 基本运算

#### 3.1. 数学运算
MiniLang 支持标准的数学运算符：`+` (加), `-` (减), `*` (乘), `/` (除), `%` (取余)。

```minilang
int a = 10;
int b = 3;
print(a + b); // Output: 13
print(a / b); // Output: 3.33333 (除法结果总是小数，以保证精度)
print(a % b); // Output: 1 (10除以3的余数是1)
```

#### 3.2. 字符串拼接
`+` 号也可以用来连接（拼接）字符串。

```minilang
string greeting = "Hello, ";
string name = "Charlie";
print(greeting + name + "!"); // Output: Hello, Charlie!
```

#### 3.3. 比较运算
我们可以比较两个值，结果总是一个 `bool` (`true` 或 `false`)。

*   `==` : 等于
*   `!=` : 不等于
*   `>` : 大于
*   `<` : 小于
*   `>=` : 大于等于
*   `<=` : 小于等于

```minilang
int my_score = 90;
int pass_score = 60;
print(my_score > pass_score);  // Output: true
print("apple" == "orange");    // Output: false
```

---

## <a name="4-做出选择-if-else-语句"></a>第二部分：控制程序的脉搏

程序不总是按顺序执行。有时，我们需要根据不同的情况做出不同的选择。

### 4. 做出选择：`if-else` 语句

`if` 语句就像一个十字路口。程序走到这里，会检查一个条件。如果条件为 `true`，就走一条路；否则，就走另一条路。

```minilang
int temperature = 25;

if (temperature > 30) {
    print("It's hot outside!");
} else if (temperature > 15) {
    print("It's a pleasant day.");
} else {
    print("It's cold, bring a jacket.");
}
// Output: It's a pleasant day.
```
*   `if (...)`: 检查括号内的条件。如果为 `true`，则执行紧跟的 `{...}` 代码块。
*   `else if (...)`: 如果前面的 `if` 条件不满足，就检查这里的条件。
*   `else`: 如果以上所有条件都不满足，就执行 `else` 的代码块。

**真值与假值 (Truthiness)**
在 `if` 的条件里，除了 `true` 本身，其他一些值也会被当作“真”。反之亦然。
*   **会被当成 `false` 的值**: `false`, `0`, `0.0`, `""` (空字符串), `nil`, `[]` (空数组), `{}` (空字典)。
*   **所有其他值**都会被当成 `true`。

### <a name="5-重复工作-循环"></a>5. 重复工作：`while` 与 `for` 循环

循环让我们可以重复执行一段代码，直到满足某个条件为止。

#### 5.1. `while` 循环
只要条件为 `true`，`while` 循环就会一直执行。

```minilang
int countdown = 3;
while (countdown > 0) {
    print(countdown);
    countdown = countdown - 1; // 关键！必须有改变条件的步骤，否则会死循环
}
print("Liftoff!");
```
**Output:**
```
3
2
1
Liftoff!
```

#### 5.2. `for` 循环
`for` 循环更适合在已知重复次数或需要遍历一个集合时使用。

**C风格 `for` 循环**
它由三部分组成：`for (初始化; 条件; 步进)`。

```minilang
for (int i = 0; i < 3; i = i + 1) {
    print("Hello number", i);
}
```
**Output:**
```
Hello number 0
Hello number 1
Hello number 2
```

**`for-each` 循环 (更常用，更推荐)**
这是遍历一个集合（如数组）所有成员的最简单方式。

```minilang
var colors = ["red", "green", "blue"];
for (var color : colors) {
    print("I like the color", color);
}
```
**Output:**
```
I like the color red
I like the color green
I like the color blue
```

#### 5.3. `break` 和 `continue`
*   `break`: 立即跳出整个循环。
*   `continue`: 跳过当前这次循环，直接进入下一次。

---

## <a name="6-代码的食谱-函数"></a>第三部分：构建坚实的程序大厦

当程序变大时，把所有代码都写在一起会变得一团糟。函数和类是组织代码、使其可复用和易于管理的利器。

### 6. 代码的“食谱”：函数

函数是一段命名的、可重复使用的代码块。你可以把它想象成一个**食谱**：它有名字（函数名），需要一些**原料**（参数），然后按照步骤执行，最后产出一个**成品**（返回值）。

```minilang
// 定义一个名为 "add" 的食谱
func add(int a, int b) {
    return a + b; // "成品"是 a 和 b 的和
}

// 使用这个食谱
int result = add(5, 3); // 提供原料 5 和 3
print("5 + 3 is", result); // Output: 5 + 3 is 8
```
*   `func`: 关键字，表示“我要定义一个函数”。
*   `add(int a, int b)`: 函数签名。`add` 是函数名，括号里是它需要的“原料”——参数列表，这里需要两个 `int` 类型的参数。
*   `{...}`: 函数体，即食谱的具体步骤。
*   `return`: 关键字，指定函数的“成品”（返回值）。函数一旦执行到 `return`，就会立即结束。

### <a name="7-整理数据-数组与字典"></a>7. 整理数据：数组与字典

#### 7.1. 数组：有序的列表

数组是一个有序的集合，就像一个购物清单。你可以通过**索引**（位置编号，从0开始）来访问其中的每个元素。

```minilang
var shopping_list = ["apples", "milk", "bread"];
print(shopping_list[0]); // Output: apples (索引从0开始)
print(shopping_list[2]); // Output: bread

shopping_list[1] = "almond milk"; // 修改第二个元素
print(shopping_list);         // Output: [apples, almond milk, bread]

append(shopping_list, "eggs"); // 在末尾添加一个元素
print(shopping_list);          // Output: [apples, almond milk, bread, eggs]

print(len(shopping_list));     // Output: 4 (获取数组的长度)
```

#### 7.2. 字典：带标签的集合

字典存储的是“键-值”对，就像一本真正的字典，你可以通过一个词（键）查到它的释义（值）。字典的键必须是字符串。

```minilang
var user = {
    "name": "David",
    "age": 42,
    "is_programmer": true
};


print(user["name"]); // 通过键 "name" 获取值. Output: David
user["age"] = 43;    // 修改值

// 也可以用点 . 操作符访问，如果键是合法的标识符
print(user.age); // Output: 43

user["city"] = "New York"; // 添加新的键值对
print(user);
```

### <a name="8-程序的蓝图-类与对象"></a>8. 程序的“蓝图”：类与对象

当我们处理更复杂的事物时，比如一个游戏里的“玩家”，他既有数据（名字、生命值），又有行为（攻击、移动）。类就是用来将这些相关的数据和行为**打包**在一起的**蓝图**。

而**对象**，就是根据这个蓝图建造出来的具体实例。

```minilang
// 定义一个 Player 的蓝图
class Player {
    // 构造函数：建造一个新 Player 对象时的说明书
    func init(name) {
        this.name = name; // this 指的是“正在被建造的这个对象”
        this.hp = 100;
    }

    // 行为（方法）
    func attack(target) {
        print(this.name + " attacks " + target + "!");
    }
}

// 根据蓝图，建造两个具体的玩家对象
var player1 = Player("Knight");
var player2 = Player("Archer");

// 调用他们的方法
player1.attack("a dragon"); // Output: Knight attacks a dragon!
player2.attack("a goblin"); // Output: Archer attacks a goblin!

print(player1.name + "'s HP is", player1.hp); // Output: Knight's HP is 100
```
*   `class`: 关键字，定义一个蓝图。
*   `init`: 特殊的**构造函数**。当你写 `Player(...)` 时，它会自动被调用。
*   `this`: 在类的方法内部，`this` 总是指向调用该方法的那个具体对象。比如 `player1.attack(...)` 被调用时，方法里的 `this` 就是 `player1`。
*   **属性**: `this.name`, `this.hp` 是对象的属性（数据）。
*   **方法**: `attack` 是对象的行为（函数）。


---

## 第四部分：成为高级工匠 (Advanced Craftsmanship)

你已经掌握了 MiniLang 的基本构件：变量、循环、函数和基础的类。现在，是时候学习如何像一位真正的工匠一样，把这些构件组织成宏伟、坚固且易于维护的程序了。

### <a name="9-组织代码-模块化"></a>9. 组织代码：`import` 与 `include` 的艺术

想象一下，你正在建造一座巨大的乐高城堡。如果把所有的砖块都堆在一个大箱子里，每次找特定的砖块都会是一场灾难。聪明的做法是把不同颜色、不同形状的砖块分门别类地放在不同的小盒子里。

在编程中，这些“小盒子”就是**模块**。MiniLang 提供了两种方式来组织你的代码模块：`include` 和 `import`。

#### 9.1. `include`: 简单的“复制粘贴”

`include` 是最直接的代码复用方式。它的行为就像把你另一个文件的代码原封不动地“复制粘贴”到当前位置。

**场景：共享的工具函数**
假设你写了一些方便的小工具，比如一个带时间戳的日志函数，你想在很多地方都用它。

**文件 `my_utils.mylang`:**
```minilang
// 这是一个工具箱文件

func log_message(string message) {
    // clock() 是一个内置函数，返回程序运行的毫秒数
    print("[" + clock() + "ms] " + message); 
}
```

**文件 `main.mylang`:**
```minilang
// 这是我们的主程序文件

log_message("Program starting..."); // 错误！现在还不知道 log_message 是什么

include "my_utils.mylang"; // “复制粘贴” my_utils.mylang 的代码到这里

log_message("Program has started successfully!"); // 现在可以用了！
```
**Output:**
```
[...ms] Program has started successfully!
```
*   **优点**: 非常简单直接。
*   **缺点/风险**: **“作用域污染”**。想象一下，如果你的 `main.mylang` 里也定义了一个叫 `log_message` 的变量，`include` 之后就会产生混乱。对于大型项目，这就像把所有乐高都倒回一个箱子，很快就会失控。
*   **使用时机**: 仅用于非常小的项目，或者注入你完全了解且确信不会冲突的全局常量或工具时。

#### 9.2. `import`: 安全的、带命名空间的“工具箱”

`import` 是 MiniLang 推荐的、更安全、更专业的模块化方式。它不会粗暴地“复制粘贴”，而是为你引入一个**独立的、带名字的工具箱**。

**场景：构建一个数学模块**

**文件 `math_lib.mylang`:**
```minilang
// 这是一个独立的数学库模块
var PI = 3.14159; // 模块内部的 PI

func circle_area(radius) {
    return PI * radius * radius;
}
```

**文件 `main.mylang`:**
```minilang
var PI = "I am a delicious pie!"; // 主程序里的 PI，和模块里的不冲突

// 引入 math_lib.mylang，并给这个工具箱起个别名叫 "math"
import "math_lib.mylang" as math;

// 要使用工具箱里的东西，必须通过它的名字
print("The value of PI from our library is:", math.PI);

float area = math.circle_area(10);
print("Area of a circle with radius 10 is:", area);

print("The other PI is still:", PI); // 主程序里的 PI 完好无损
```
**Output:**
```
The value of PI from our library is: 3.14159
Area of a circle with radius 10 is: 314.159
The other PI is still: I am a delicious pie!
```
*   **`import "path" as alias`**: `import` 关键字告诉 MiniLang 去加载一个模块，`as` 关键字给这个模块的工具箱起一个本地的昵称。
*   **优点**:
    *   **命名空间 (Namespace)**: 所有模块成员都必须通过 `alias.` 来访问，绝不会与你的代码产生命名冲突。
    *   **隔离性**: 模块在一个独立的沙箱环境中运行，它不能访问你主程序的局部变量，反之亦然。这保证了代码的纯净和可预测性。
    *   **缓存**: 同一个模块无论被 `import` 多少次，都只会被执行一次。
*   **结论**: **永远优先使用 `import`**。它是构建可靠、可维护的大型应用的基石。

### <a name="10-优雅地处理错误"></a>10. 优雅地处理错误：`try-catch` 机制

程序并不总是按计划运行。用户可能会输入错误的数据，文件可能不存在，网络可能会中断。一个健壮的程序不应该在遇到这些问题时崩溃，而应该能优雅地处理它们。

`try-catch` 就是我们的“安全网”。

#### 10.1. 抛出与捕获自定义错误
想象你在写一个银行取款函数。如果余额不足，程序应该停止取款，并给出一个明确的错误信息，而不是继续执行下去。

```minilang
func withdraw(float current_balance, float amount) {
    if (amount > current_balance) {
        // 余额不足，情况不妙！抛出一个错误！
        throw "Insufficient funds!";
    }
    return current_balance - amount;
}

float my_balance = 100.0;
print("Balance is:", my_balance);

// 使用安全网来尝试执行可能出错的操作
try {
    print("Attempting to withdraw 50...");
    my_balance = withdraw(my_balance, 50.0);
    print("Success! New balance:", my_balance);

    print("Attempting to withdraw 80...");
    my_balance = withdraw(my_balance, 80.0); // 这行会抛出错误
    print("This line will never be reached.");

} catch (error_message) { // 如果 try 块中任何地方抛出错误，程序会立即跳到这里
    print("--- OPERATION FAILED ---");
    print("Reason:", error_message);
    print("Balance remains:", my_balance);
}

print("Program continues after handling the error.");
```
**Output:**
```
Balance is: 100.0
Attempting to withdraw 50...
Success! New balance: 50.0
Attempting to withdraw 80...
--- OPERATION FAILED ---
Reason: Insufficient funds!
Balance remains: 50.0
Program continues after handling the error.
```
*   `try { ... }`: 把你认为可能“踩雷”的代码放在这个块里。
*   `throw ...`: 当错误发生时，使用 `throw` 关键字“扔出”一个错误信号。这个信号可以是一个字符串，或任何其他值。
*   `catch (variable) { ... }`: `try` 块的“安全网”。一旦接到 `throw` 出来的信号，它就会被激活，那个信号（我们扔出的 `"Insufficient funds!"`）会被装进 `catch` 的括号里的变量中。

#### 10.2. 捕获解释器自身的错误
`try-catch` 更加强大之处在于，它还能捕获 MiniLang 解释器自己产生的运行时错误，比如除以零、类型不匹配等。

当你捕获一个内部错误时，你得到的不再是一个简单的字符串，而是一个包含了更多信息的**错误对象**，它有 `message` 和 `line` 两个属性。

```minilang
try {
    print("About to do something impossible...");
    var result = 10 / 0; // 这会产生一个内部运行时错误
} catch(err_obj) {
    print("Caught an internal error!");
    print("Error message:", err_obj.message);
    print("It happened on line:", err_obj.line);
}
```
这对于创建非常可靠的程序至关重要，你甚至可以构建一个框架，自动记录所有未被捕获的内部错误，并将其写入日志文件。

### <a name="11-深度探索-高级主题"></a>11. 深度探索：高级主题与设计哲学

这一章，我们将深入 MiniLang 的“引擎室”，探讨一些能让你代码水平产生质的飞跃的高级概念。

#### 11.1. 闭包：带记忆的函数
函数不仅能执行代码，还能“记住”它被创建时周围的环境。这个特性叫做**闭包 (Closure)**。

想象一个能制造“计数器”的工厂函数：

```minilang
func make_counter() {
    int count = 0; // 这个 count 变量属于 make_counter 的环境

    // 我们返回一个新的、匿名的函数
    // 这个新函数“捕获”并“记住”了上面的 count
    return func() {
        count = count + 1;
        return count;
    };
}

var counterA = make_counter(); // counterA 是一个带自己独立记忆的计数器
var counterB = make_counter(); // counterB 也有自己的独立记忆

print(counterA()); // Output: 1
print(counterA()); // Output: 2
print(counterB()); // Output: 1 (counterB 的记忆和 A 是分开的)
print(counterA()); // Output: 3
```
闭包是实现高阶函数、回调和许多优雅编程模式的基础。

#### 11.2. 继承的深化：`extends` 与 `super`
我们已经知道 `class` 是蓝图。**继承**则允许我们基于一个旧蓝图，创建一个新的、更详细的蓝图。

**文件 `creature.mylang`:**
```minilang
class Creature {
    func init(name, hp) {
        this.name = name;
        this.hp = hp;
    }
    func take_damage(amount) {
        this.hp = this.hp - amount;
        print(this.name + " takes " + amount + " damage, HP is now " + this.hp);
    }
}
```
**文件 `main.mylang`:**
```minilang
include "creature.mylang"; // 这里用 include 方便演示

// Dragon 是一个 Creature，所以我们继承它
class Dragon extends Creature {
    // Dragon 有 Creature 的所有属性和方法，但我们还想加点新东西
    func init(name) {
        // 我们需要调用父蓝图(Creature)的构造函数来完成基础部分的建造
        super.init(name, 500); // super 指向父类，这里调用 Creature.init
        this.fire_breath_ready = true;
    }

    func breathe_fire() {
        if (this.fire_breath_ready) {
            print(this.name + " unleashes a torrent of fire!");
            this.fire_breath_ready = false;
        } else {
            print(this.name + " needs to catch its breath.");
        }
    }
}

var smaug = Dragon("Smaug");
smaug.take_damage(50); // 调用从 Creature 继承来的方法
smaug.breathe_fire();  // 调用 Dragon 自己独有的方法
```
*   `extends Creature`: 表示 `Dragon` 蓝图基于 `Creature` 蓝图。
*   `super.init(...)`: 在子类的构造函数中，使用 `super` 来调用父类的构造函数，完成“地基”部分的初始化。这是非常重要的一步。

#### 11.3. 双范式对象：`class` vs. `Object()`
MiniLang 提供两种创建对象的方式，适用于不同场景。
*   **`class`**: 你已经很熟悉了。它是**类式范式**，结构严谨，先有蓝图再有实例。是构建应用程序主体框架的首选。
*   **`Object()`**: 这是**原型式范式**，更灵活、更动态。它允许一个对象直接从另一个对象继承，无需先定义类。

**场景：游戏里的魔法物品**
一个基础魔法戒指，所有其他戒指都基于它来制作。

```minilang
// 基础原型
var base_magic_ring = Object();
base_magic_ring.material = "silver";
base_magic_ring.enchantment_level = 1;
base_magic_ring.identify = func() {
    print("A " + this.material + " ring, glowing faintly.");
};

// Ring of Strength 直接从原型创建，并添加/覆盖属性
var ring_of_strength = Object(base_magic_ring);
ring_of_strength.bonus = "+5 Strength";
ring_of_strength.identify = func() {
    // 调用原型的 identify 方法
    super.identify(); // 没错, Object() 创建的对象也可以用 super!
    print("It grants a bonus of " + this.bonus);
};

ring_of_strength.identify();
```
`Object()` 更适合用于创建一次性的、高度动态的或配置性的对象，是对 `class` 模式的完美补充。

---

## <a name="12-内置函数大全"></a>第五部分：实用工具箱 (The Reference Toolkit)

这里是 MiniLang 内置的“瑞士军刀”，这些函数全局可用，能帮你完成各种常见任务。

### 12. 内置函数大全

#### I/O & 转换
*   `print(...)`: 打印一个或多个值到控制台，值之间用空格隔开。
*   `input([prompt])`: `string input(string prompt)` - 显示可选的 `prompt` 提示信息，并等待用户输入一行文本，返回该文本字符串。
*   `str(v)`: `string str(any)` - 将任何类型的值转换为其字符串表示形式。
*   `int(v)`: `int int(any)` - 尝试将一个值转换为整数。可以转换数字、布尔值和数字内容的字符串。
*   `float(v)`: `float float(any)` - 尝试将一个值转换为浮点数。
*   `bool(v)`: `bool bool(any)` - 将一个值转换为其布尔“真值”。

#### 数据结构操作
*   `len(obj)`: `int len(string|array|dict|object)` - 返回字符串的长度、数组的元素个数、或字典/对象的键值对数量。
*   `append(arr_or_str, val)`: `array|string append(...)` - 如果第一个参数是数组，则将 `val` 追加到数组末尾（原地修改）。如果是字符串，则将 `val` 的字符串形式拼接到末尾（返回新字符串）。
*   `pop(arr, [idx])`: `any pop(array, int idx)` - 移除并返回数组中的一个元素。如果不提供 `idx`，则移除并返回最后一个元素。
*   `range(stop)` / `range(start, stop, [step])`: `array range(...)` - 创建一个整数数组。例如 `range(3)` -> `[0, 1, 2]`; `range(1, 4)` -> `[1, 2, 3]`.
*   `keys(dict_or_obj)`: `array keys(dict|object)` - 返回一个包含字典或对象所有键的数组。
*   `del(dict_or_obj, key)`: `nil del(...)` - 从字典或对象实例中删除一个键值对。

#### 内省与高级工具
*   `type(v)`: `string type(any)` - 返回一个值的类型的字符串描述，如 `"int"`, `"string"`, `"MyClass"`, `"object"`.
*   `deepcopy(v)`: `any deepcopy(any)` - 创建一个值的完整、独立的深拷贝。对于嵌套的数组和字典尤其有用，能防止“别名效应”并能处理循环引用。
*   `has(dict_or_obj, key)`: `bool has(...)` - 检查一个字典或对象（包括其原型链）是否拥有指定的键。
*   `dir(obj)`: `array dir(dict|object)` - 返回一个对象所有可访问属性名（包括继承的）的数组。非常适合调试。
*   `assert(cond, [msg])`: `nil assert(...)` - 如果 `cond` 为假，则程序立即因断言失败而终止，并显示可选的 `msg`。

#### 函数式编程
*   `map(func, arr)`: `array map(function, array)` - 接受一个函数和一个数组，对数组的每个元素调用该函数，并返回一个包含所有返回结果的新数组。
*   `filter(func, arr)`: `array filter(function, array)` - 接受一个返回布尔值的函数和一个数组，返回一个新数组，其中只包含那些让函数返回 `true` 的原始元素。

#### 文件与系统
*   `read_file(path)`: `string read_file(string)` - 读取并返回一个文件的全部内容作为字符串。
*   `write_file(path, content)`: `nil write_file(string, string)` - 将 `content` 字符串写入到指定 `path` 的文件中，会覆盖旧文件。
*   `clock()`: `int clock()` - 返回自程序启动以来经过的毫秒数。

### <a name="13-语法速查表"></a>13. 语法速查表

这份速查表是你编程时的快速参考。

```
// 变量
var name = value;
int i = 0; string s = ""; bool b = true;

// 控制流
if (condition) { ... } else if (condition) { ... } else { ... }
while (condition) { ... }
for (var i=0; i<N; i=i+1) { ... }
for (var item : collection) { ... }
break; continue;

// 函数
func name(param1, param2) {
    return value;
}

// 类
class Name extends Parent {
    func init(p1) {
        super.init(...);
        this.field = p1;
    }
    func method() { ... }
}

// 模块
import "path" as alias;
include "path";

// 异常
try { ... } catch (e) { ... }
throw value;

// 数据结构字面量
var arr = [1, "two", true];
var dict = {"key1": value1, "key2": 2};
```

