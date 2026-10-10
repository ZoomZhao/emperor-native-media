# 本次提交的来源与验收边界

本提交收录 Codex 在 Antigravity 接手前已完成的独立修复、可迁移测试入口和文档。核心源码与 `portable-reproduction-receipt.json` 的 33 项源码哈希逐字一致，依赖齐全；交接前的干净导出在无关 cwd、含空格路径中完成七种配置验证：data 的 Release/UBSAN 各91断言与11项TLV测试，media 各65,691断言，AppMedia/fake 集成各62断言，以及 App 的 Release 只构建、签名和系统链接检查。App 未启动，未使用音频设备或 GameData。

五个混合文件仅提交交接前冻结版本：`game_lab/app_media.hpp`、`game_lab/app_media.cpp`、`game_lab/app/av_audio_backend.mm`、`game_lab/app/main.mm`、`game_lab/app_media_integration_tests.mm`。Antigravity 的完成/错误回调、ticket、额外资源入口和90断言测试仍保留在工作区，未包含在本提交中。暂存通过明确路径和既有字节生成，不覆盖工作区。

其余文档与交接前保留副本逐字匹配；只在提交版本中删除个人绝对路径，并把未纳入的历史报告链接标为本地报告。原文档内“未提交”“未执行”等表述属于各自验证时点，应以本文件和对应凭据的源码哈希判断范围。

因缺少独立保存的当前版本归属凭据，本次保留未提交：`game_lab/run.sh`、`game_lab/boundary_tests.mm`、`runtime_core/dependency-map.json`、`docs/SOURCE_REPAIR.md`、`docs/source-repair-static-audit.json`、`docs/RUNTIME_PREPARATION.md`、`docs/runtime-preparation-static-audit.json`。旧 GameData 样例入口不属于本次发布验收入口；验证应使用 `scripts/independent_runner.py` 或对应纯自造 wrapper。没有声称旧样例入口已在本提交中复验可用。

GameData、原二进制、衍生产物、源码导出目录、构建输出、凭据和个人本地路径均未加入。本提交不包含完整游戏引擎或视频解码实现，不恢复此前受限的原执行链。
