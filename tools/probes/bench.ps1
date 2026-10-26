param([int]$Bots = 100, [int]$Seconds = 15)

$server = Get-Process server -ErrorAction Stop
$load = Start-Job -ScriptBlock { param($b, $s) Set-Location C:\; python -I "C:\New folder (2)\Minecraft Server\tools\probes\load.py" $b $s 2>&1 } -ArgumentList $Bots, $Seconds

$cpu0 = $server.TotalProcessorTime.TotalMilliseconds
$t0 = Get-Date
$peak = 0
$samples = @()
while ($load.State -eq 'Running') {
    Start-Sleep -Milliseconds 1000
    $server.Refresh()
    $peak = [math]::Max($peak, $server.WorkingSet64)
    $samples += $server.TotalProcessorTime.TotalMilliseconds
}
$elapsed = ((Get-Date) - $t0).TotalMilliseconds
$server.Refresh()
$cpu = $server.TotalProcessorTime.TotalMilliseconds - $cpu0

Receive-Job $load
Remove-Job $load
"server cpu {0:N0} ms over {1:N1} s = {2:N2}% of one core" -f $cpu, ($elapsed / 1000), (100 * $cpu / $elapsed)
"server peak memory {0:N1} MB, threads {1}, handles {2}" -f ($peak / 1MB), $server.Threads.Count, $server.HandleCount
