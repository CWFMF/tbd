import datetime

# HACK: just get the values out of here for now
from common import CONCURRENT_SIMS, CONFIG

_BATCH_ACCOUNT_NAME = CONFIG.get("BATCH_ACCOUNT_NAME")
_BATCH_ACCOUNT_KEY = CONFIG.get("BATCH_ACCOUNT_KEY")
_STORAGE_ACCOUNT_NAME = CONFIG.get("STORAGE_ACCOUNT_NAME")
_STORAGE_CONTAINER = CONFIG.get("STORAGE_CONTAINER")
_STORAGE_KEY = CONFIG.get("STORAGE_KEY")
_REGISTRY_USER_NAME = CONFIG.get("REGISTRY_USER_NAME")
_REGISTRY_PASSWORD = CONFIG.get("REGISTRY_PASSWORD")

# _POOL_VM_SIZE = "STANDARD_F72S_V2"
_POOL_VM_SIZE = "STANDARD_F32S_V2"
# _POOL_VM_SIZE = "STANDARD_HB120RS_V2"
_POOL_ID_BOTH = "pool_firestarr_both"
_POOL_ID_PY = "pool_firestarr_py"
_POOL_ID_BIN = "pool_firestarr_bin"
_MIN_NODES = 0
_MAX_NODES = 25
_AUTO_SCALE_FORMULA = "\n".join(
    [
        f"$min_nodes = {_MIN_NODES};",
        f"$max_nodes = {_MAX_NODES};",
        # "$usage = $CPUPercent.GetSample(TimeInterval_Minute * 3, 20);",
        # "$usage_avg = avg($usage);",
        # "$min_nodes = $usage_avg < (1 / 72 * 0.8) ? 0 : 1;",
        "$samples = $ActiveTasks.GetSamplePercent(TimeInterval_Minute);",
        "$tasks = $samples < 1 ? 0 : $ActiveTasks.GetSample(1);",
        "$TargetDedicatedNodes = max($min_nodes, min($max_nodes, $tasks));",
        "$NodeDeallocationOption = taskcompletion;",
    ]
)
_AUTO_SCALE_EVALUATION_INTERVAL = datetime.timedelta(minutes=5)
_BATCH_ACCOUNT_URL = f"https://{_BATCH_ACCOUNT_NAME}.canadacentral.batch.azure.com"
_STORAGE_ACCOUNT_URL = f"https://{_STORAGE_ACCOUNT_NAME}.blob.core.windows.net"
_REGISTRY_SERVER = f"{_REGISTRY_USER_NAME}.azurecr.io"
_CONTAINER_PY = f"{_REGISTRY_SERVER}/firestarr/tbd_prod_stable:latest"
_CONTAINER_BIN = f"{_REGISTRY_SERVER}/firestarr/firestarr:latest"