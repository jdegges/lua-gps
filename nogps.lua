require 'gps'
require 'posix'

g = gps.open ('localhost', '2947')
g:stream (gps.WATCH_ENABLE)

while true do
  while true == g:waiting (4000000) do
    d = g:read ()
    print (d.fix.latitude .. "° ± " .. d.fix.epy .. " m",
           d.fix.longitude .. "° ± " .. d.fix.epx .. " m")
  end

  print ("some kind of error... retrying after 10 seconds")
  posix.sleep (10)

  g:close()
  g = gps.open ('localhost', '2947')
  g:stream (gps.WATCH_ENABLE)
end
