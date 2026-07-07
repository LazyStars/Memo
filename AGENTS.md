# 项目说明

## 项目说明：
本项目使用Qt5.15.2 + C++17 + CMake，若没有明确要求，不需要使用其他Qt版本的兼容性写法。代码编写后不需要做CMake编译检查，仅做静态代码检查，并且检查是否违反下面的审查规则。

目标平台：
- Windows（MSVC 2022）  

多平台支持下，具有平台相关性API需要明确使用#ifdef Q_OS_WIN或者Q_OS_LINUX选择性使用代码。

## 代码生成规则

- 优先保持现有架构风格
- 优先局部修改，不要大范围重构
- 除非明确要求，否则不要修改公共接口
- 不要引入新的第三方库
- 不要修改无关文件
- 优先修改现有类
- 优先复用现有工具类
- 不主动创建新框架
- 不主动创建新的基类体系
- 不主动创建新的线程封装
- 不主动创建新的路由机制

## 搜索原则

实现功能前优先搜索功能模块。

确认不存在现有能力后再新增。

## 标准库与Qt类型使用

- 本项目优先保持 Qt5 代码风格和 Qt 类型体系，已有 Qt 容器、工具类型和算法可满足需求时，优先使用 Qt 支持的写法，例如 `QString`、`QList`、`QVector`、`QMap`、`QHash`、`QPair`、`QVariant` 等。
- 除非 Qt 官方已经明确标记为 `QT_DEPRECATED` 或当前 Qt 版本不推荐继续使用，否则不要主动把既有 Qt 写法替换为 `std` 写法。例如 `qSort` 已废弃时可改用 `std::sort`，但 `QPair`、`QList` 等未废弃类型不需要替换为 `std::pair`、`std::vector`。
- 与 Qt API、信号槽、`QVariant`、元对象系统、model/view 数据交互相关的代码，优先使用 Qt 类型，减少 Qt/std 类型之间的无意义转换。
- 只有在标准库能力明显更合适、Qt 没有等价能力、或现有代码上下文已经使用标准库类型时，才局部使用 `std` 类型，并避免扩大公共接口变更。

## 线程使用

- 局部、一次性、临时后台任务优先使用 `QtConcurrent::run(...)` 或 `QThread::create(...)` 创建 lambda 线程，例如启动流程中短时间执行的耗时初始化、文件处理、接口外的计算任务等。
- 使用 `QtConcurrent` 时需要引用 `<QtConcurrent/QtConcurrent>`，任务函数内不要直接操作 UI；需要回到主线程更新界面时，使用 signal/slot 或 `QMetaObject::invokeMethod(..., Qt::QueuedConnection)`。
- 使用 `QThread::create` 时必须明确管理 `QThread` 生命周期：创建后连接 `finished` 到 `deleteLater`，必要时指定 parent 或保存指针；对象析构、页面关闭、应用退出时要确保线程已经退出，避免 lambda 捕获的 `this` 或 UI 对象悬空。

```cpp
auto* thread = QThread::create([this] {
    // do work...
    QMetaObject::invokeMethod(this, [this] {
        // update ui or vm in main thread
    }, Qt::QueuedConnection);
});
connect(thread, &QThread::finished, thread, &QObject::deleteLater);
thread->start();
```

- 持续运行、生命周期较长、需要在应用退出时统一通知退出的线程，不要手写裸 `QThread` 管理，使用项目线程管理封装创建。项目现有封装位于 `src/modules/module-view-comm/utils/threadmanage.h`，通过 `ThreadManage::create<T>(...)` 创建线程实例，由管理器在应用退出时统一 `quit` 并等待释放。
- 持续计算型线程继承 `ComputeThread` 并重写 `run()`；循环内必须定期检查 `isInterruptionRequested()`。如果线程阻塞在等待条件上，需要重写 `requestToQuit()` 唤醒等待。
- 基于事件循环的常驻线程继承 `EventDrivenThread`，对象会被移动到内部工作线程，耗时 slot 中如有循环同样需要检查 `thr->isInterruptionRequested()`。退出时调用线程对象的 `quit()`，不要直接跨线程删除工作对象。
- 线程内不要直接访问或修改 QWidget/UI 控件；跨线程传递数据优先使用信号槽、事件总线或 `QMetaObject::invokeMethod` 的 queued 调用。需要传递自定义类型时先按项目既有方式 `Q_DECLARE_METATYPE` / `qRegisterMetaType` 注册。

