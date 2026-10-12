# 共享前缀独立回归

基线 `fe09b158e410c232b6820932a6d9dc878d11a976`。所有数据为自造内存输入；没有读取实际 payload，没有执行条件实验工具或原程序。

8组测试在基线上 **Release7/1、UBSan7/1**。唯一失败是文档规定的累计 FSA 追加≤1024没有在初始缓存验证中实施：prefix6020加1025个合法FSA Class条目（总7045、低于8192）时，一个不再追加的NULL记录成功返回。检查目前只在 append() 时执行；初始1024边界成功。该组故意保留红色，不能把基线报告写成全部通过。

其余7组验证：完整前缀字段/编号/种类/class关系/schema复制；6020/6021注册及随后引用；同视图下一记录继续沿用全局ID；四次记录累积1024后第五次追加拒绝；Opaque不冒充FSA；自造未知名诊断不进入payload；晚期失败保留完整旧输出与输入cache；错误初始class关系拒绝。

期望来源逐项在 `expectations.json`。资源数字为现代合同，不是原游戏数值。自造名称 `FigureFSAState` 是公开给定的候选字符串；测试使用独立夹具 `demo!` 形成相对tag12/name18/end32。它不复用实际数据，也不把位置一致或名称相同当作类型证明。

```sh
python3 file_io/tests/figurefsa_prefix_independent/run.py --source-root . --output /tmp/figurefsa-prefix-independent-next
```

总预算180秒、编译60秒、每次执行10秒。基线退出1是预期的回归失败；集成方修补初始追加量校验后可复跑。没有生产代码修补。

同一补丁只修复现有 `figurefsa_prefix_chain/run.py` 的 Make depfile 解析（它与 b407 对象驱动原字节相同，继承已复现的引号路径问题）；生产头保持原字节。未执行实际输入实验，条件和无条件前沿仍1991913，新真实解释0字节。
