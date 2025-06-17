# MiniLang
# MiniLang

MiniLang 是一种功能丰富、易于学习的编程语言，旨在为开发者提供简洁高效的编程体验。它支持变量定义、函数调用、面向对象编程等常见编程概念，适用于脚本编写、快速原型开发等多种场景。

## 快速开始

### 使用示例
创建一个名为 `test.minilang` 的文件，内容如下：
```minilang
var message = "Hello, MiniLang!";
print(message);
```
你可以使用 MiniLang 解释器运行该程序，目前，解释器是MiniLang.cpp，请你在main函数中找到 `std::string_view program = ` 语句，在 `R"CODE(` 与 `)CODE";` 之间直接粘贴你的代码。
我们承诺会在今后的版本中推出解释器和编译器（后者可能需要较长时间）。

## 语言特性
最详细的语言特性在 MiniLangDoc.md 中可见。
- **丰富的字面量类型**：支持整数、浮点数、字符串、布尔值、数组、字典和空值字面量。
```minilang
var num = 123;
var pi = 3.14;
var str = "Hello";
var isTrue = true;
var arr = [1, 2, 3];
var dict = {"key": "value"};
var nilValue = nil;
```
- **强大的面向对象能力**：支持类的定义、继承和方法重写。
```minilang
class Animal {
    func init(name) {
        this.name = name;
    }
    func speak() {
        print(this.name + " makes a sound.");
    }
}

class Dog extends Animal {
    func speak() {
        print(this.name + " barks.");
    }
}

var dog = Dog("Buddy");
dog.speak();
```
- **内置函数**：提供了一系列内置函数，如 `print`、`len`、`type` 等。
```minilang
var arr = [1, 2, 3];
print(len(arr)); // 输出 3
print(type(arr)); // 输出 array
```

## 代码示例
### 购物程序示例
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
```

## 文档链接
详细的语言规范请参考 [MiniLang 语言规范 (v1.2.1) - 深度详尽版](MiniLangDoc.md)。

## 贡献指南
我们欢迎社区的贡献！如果你想为 `MiniLang` 项目做出贡献，请遵循以下步骤：
1. Fork 本仓库。
2. 创建一个新的分支：`git checkout -b your-feature-branch`。
3. 提交你的更改：`git commit -m "Add your feature"`。
4. 推送分支：`git push origin your-feature-branch`。
5. 发起 Pull Request。

## 许可证信息
本项目采用 [MIT 许可证](LICENSE)。 
