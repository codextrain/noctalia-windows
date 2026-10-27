$ErrorActionPreference = 'Stop'
[Console]::OutputEncoding = [System.Text.UTF8Encoding]::new($false)
$rows = [System.Collections.Generic.List[object]]::new()
function Add-Info($label, $value) {
    if (-not [string]::IsNullOrWhiteSpace([string]$value)) {
        $rows.Add(@{label=$label; value=[string]$value})
    }
}
function Read-Components($class, $action) {
    try { Get-CimInstance -ClassName $class -OperationTimeoutSec 4 | ForEach-Object $action }
    catch { Add-Info $class 'Não foi possível consultar este componente.' }
}
Read-Components Win32_Processor {
    Add-Info 'Processador' ($_.Name.Trim() + ' · ' + $_.NumberOfCores + ' núcleos / ' + $_.NumberOfLogicalProcessors + ' threads')
}
Read-Components Win32_VideoController { Add-Info 'Placa de vídeo' $_.Name }
Read-Components Win32_ComputerSystem {

    Add-Info 'Computador' (($_.Manufacturer + ' ' + $_.Model).Trim())
}
try {
    $capacity = (Get-CimInstance Win32_PhysicalMemory -OperationTimeoutSec 4 | Measure-Object Capacity -Sum).Sum
    if ($capacity -gt 0) { Add-Info 'Memória instalada' ('{0:N1} GB' -f ($capacity / 1GB)) }
} catch { Add-Info 'Memória instalada' 'Não foi possível consultar a memória.' }
Read-Components Win32_BaseBoard { Add-Info 'Placa-mãe' (($_.Manufacturer + ' ' + $_.Product).Trim()) }
Read-Components Win32_DiskDrive { Add-Info 'Armazenamento' ($_.Model + ' · ' + ('{0:N0} GB' -f ($_.Size / 1GB))) }
Read-Components Win32_OperatingSystem { Add-Info 'Windows' ($_.Caption + ' · ' + $_.OSArchitecture + ' · build ' + $_.BuildNumber) }
ConvertTo-Json -InputObject @($rows.ToArray()) -Depth 3 -Compress
