# PowerShell · Painel-NiriWin.ps1 · painel de controle · Windows 11 · Windows PowerShell 5.1
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing
[System.Windows.Forms.Application]::EnableVisualStyles()
$script:base = $PSScriptRoot
$script:exe = Join-Path $script:base 'niriwin.exe'
$script:state = Join-Path $script:base 'state'
New-Item -ItemType Directory -Force -Path $script:state | Out-Null
$script:stderr = Join-Path $script:state 'manager-error.log'
$script:stdout = Join-Path $script:state 'manager.log'
$script:busy = $false
$form = New-Object System.Windows.Forms.Form
$form.Text = 'NiriWin - controle'
$form.ClientSize = New-Object System.Drawing.Size(540, 320)
$form.StartPosition = 'CenterScreen'
$form.FormBorderStyle = 'FixedDialog'
$form.MaximizeBox = $false
$label = New-Object System.Windows.Forms.Label
$label.Location = New-Object System.Drawing.Point(20, 18)
$label.Size = New-Object System.Drawing.Size(500, 55)
$label.Font = New-Object System.Drawing.Font('Segoe UI', 12)
$form.Controls.Add($label)
$details = New-Object System.Windows.Forms.TextBox
$details.Location = New-Object System.Drawing.Point(20, 80)
$details.Size = New-Object System.Drawing.Size(500, 90)
$details.Multiline = $true
$details.ReadOnly = $true
$details.ScrollBars = 'Vertical'
$form.Controls.Add($details)
$hint = New-Object System.Windows.Forms.Label
$hint.Location = New-Object System.Drawing.Point(20, 180)
$hint.Size = New-Object System.Drawing.Size(500, 50)
$hint.Text = "Navegar: Alt + setas esquerda/direita.`r`nFechar este painel mantem o NiriWin ativo. Use Parar para desligar."
$form.Controls.Add($hint)
function Get-Manager {
    @(Get-Process -Name niriwin -ErrorAction SilentlyContinue | Where-Object {
        try { $_.Path -eq $script:exe } catch { $false }
    }) | Select-Object -First 1
}
function Send-Control([string]$argument) {
    $result = & $script:exe $argument 2>&1
    if ($LASTEXITCODE -ne 0) { throw ($result -join "`r`n") }
}
function Start-Manager {
    if (Get-Manager) { return }
    if ((Test-Path -LiteralPath (Join-Path $script:state 'recovery.tsv')) -or (Test-Path -LiteralPath (Join-Path $script:state 'blur-recovery.json')) -or (Test-Path -LiteralPath (Join-Path $script:state 'taskbar-recovery.json'))) {
        Send-Control '--recover'
    }
    Start-Process -FilePath $script:exe -ArgumentList '--run' -WorkingDirectory $script:base -WindowStyle Hidden -RedirectStandardOutput $script:stdout -RedirectStandardError $script:stderr | Out-Null
}
function Update-Panel {
    $manager = Get-Manager
    if ($manager) {
        $label.Text = 'ATIVO - aguardando janelas'
        try {
            $status = Get-Content -LiteralPath (Join-Path $script:state 'status.json') -Raw | ConvertFrom-Json
            if ($status.paused) { $label.Text = 'PAUSADO - janelas restauradas' }
            elseif ($null -ne $status.managed) { $label.Text = "ATIVO - $($status.managed) janela(s), $($status.monitors) monitor(es)" }
        } catch { }
        $details.Text = 'Maximizadas e tela cheia ficam fora do gerenciamento. Clique numa janela comum antes de usar as setas.'
    } else {
        $label.Text = 'PARADO'
        $errorText = if (Test-Path -LiteralPath $script:stderr) { Get-Content -LiteralPath $script:stderr -Raw } else { '' }
        if ($errorText) { $details.Text = $errorText }
        elseif (Test-Path -LiteralPath (Join-Path $script:state 'recovery.tsv')) { $details.Text = 'A execucao anterior foi interrompida. Iniciar recupera as janelas e ativa o gerenciador.' }
        else { $details.Text = 'Clique em Iniciar para organizar as janelas.' }
    }
}
$specs = @(@('Iniciar', 20), @('Pausar', 145), @('Retomar', 270), @('Parar', 395))
foreach ($spec in $specs) {
    $button = New-Object System.Windows.Forms.Button
    $button.Text = $spec[0]
    $button.Location = New-Object System.Drawing.Point($spec[1], 245)
    $button.Size = New-Object System.Drawing.Size(115, 36)
    $button.Add_Click({
        if ($script:busy) { return }
        $script:busy = $true
        try {
            switch ($this.Text) {
                'Iniciar' { Start-Manager }
                'Pausar' { if (Get-Manager) { Send-Control '--pause' } }
                'Retomar' { if (Get-Manager) { Send-Control '--resume' } else { Start-Manager } }
                'Parar' { if (Get-Manager) { Send-Control '--stop' } }
            }
        } catch {
            [System.Windows.Forms.MessageBox]::Show($_.Exception.Message, 'NiriWin - erro') | Out-Null
        } finally { $script:busy = $false }
        Update-Panel
    })
    $form.Controls.Add($button)
}
$timer = New-Object System.Windows.Forms.Timer
$timer.Interval = 750
$timer.Add_Tick({ if (-not $script:busy) { Update-Panel } })
$timer.Start()
$form.Add_Shown({
    try { Start-Manager } catch { [System.Windows.Forms.MessageBox]::Show($_.Exception.Message, 'NiriWin - erro') | Out-Null }
    Update-Panel
})
$form.Add_FormClosing({
    $timer.Stop()
})
[void]$form.ShowDialog()
$timer.Dispose()
$form.Dispose()
