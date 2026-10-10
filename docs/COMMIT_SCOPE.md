# 本次提交的来源与验收边界

本次是在已确认远端9d8e2db上的独立发布 checkout 合入，原活动目录与索引保持不变。前置增量来自已冻结、审查的五个自写源码文件：AppMedia、原生音频 delegate、App资源接线与自造集成用例。新增修复是同步终止回调后 false 返回的 ticket/状态/所有权处理；新增可携带生命周期测试与明确来源/验收文档。

最终源码重新通过原四项、六反例、扩展生命周期 Release/UBSAN、AppMedia集成各90断言以及App只构建/签名/系统链接检查。详见 [CALLBACK_REPAIR.md](CALLBACK_REPAIR.md) 和绑定源码哈希的 [callback-release-receipt.json](callback-release-receipt.json)。既有数据/图形源码未改，既有验收不重复计入本轮。

明确排除活动目录中的 game_lab/run.sh、game_lab/boundary_tests.mm、runtime_core/dependency-map.json、docs/SOURCE_REPAIR.md、docs/source-repair-static-audit.json、docs/RUNTIME_PREPARATION.md、docs/runtime-preparation-static-audit.json。其余活动文档变动也没有整份复制。历史文件保留在Git基线，不恢复受阻执行链。

没有新增GameData、游戏资产、原二进制、衍生游戏库、生成程序/包、密钥或个人本地路径。实际输入和完整日志留在独立本地结果目录，仓库仅提交可复现源码与去除本地路径的摘要凭据。原活动目录没有伪装成已同步。
