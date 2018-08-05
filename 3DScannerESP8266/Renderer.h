R"bitluni(
<html>
  <header>
  <script>
//inspired by:
//https://blogoben.wordpress.com/2011/04/16/webgl-basics-4-wireframe-3d-object/ 
  
  fragmentShaderCode=
  'varying mediump float d;' +
  'void main(void)' + 
  '{' +
  '  gl_FragColor = vec4(1.0 - d, 0.0, d, 1.0);' +
  '}';

  vertexShaderCode=
  'uniform mat4 mvp;' +
  'uniform float scaler;' +
  'attribute vec3 ppos;' +
  'varying mediump float d;' +
  'void main(void)' +
  '{' +
  //'  vec3 v = ppos * scaler;' +
  '  vec4 v = mvp * vec4(ppos.x, ppos.y, ppos.z, 1.0);' +
  '  gl_Position = mvp * vec4(ppos.x, ppos.y, ppos.z, 1.0);' +
  '  d = sqrt(dot(v.xyz, v.xyz));' +
  '  gl_PointSize = 3.0;' +
  '}';

// Global variables
//-----------------------
var gl = null; // GL context
var program; // The program object used in the GL context
var running = true; // True when the canvas is periodically refreshed

var method = 1;
var scale = 1;

function methodChange()
{
  method = method ^ 1;  
}

//var vertices;
function start()
{
  //convert vertices
  var m = 0.01;
  for(var i = 0; i < vertices.length; i += 3)
  {
    var d = Math.sqrt(vertices[i] * vertices[i] + vertices[i + 1] * vertices[i + 1] + vertices[i + 2] * vertices[i + 2]);
    m = Math.max(m, d);
  }
  scale = 1 / m;
  
  var canvas = document.querySelector('canvas');

  try 
  {
  gl = canvas.getContext('experimental-webgl');
  }
  catch(e) 
  {
  alert('Exception catched in getContext: '+e.toString());
  return;
  }
  
  if(!gl) return;
  
  var fshader = gl.createShader(gl.FRAGMENT_SHADER);
  gl.shaderSource(fshader, fragmentShaderCode);
  gl.compileShader(fshader);
  if (!gl.getShaderParameter(fshader, gl.COMPILE_STATUS)) 
  {alert('Error during fragment shader compilation:\n' + gl.getShaderInfoLog(fshader)); return;}


  var vshader = gl.createShader(gl.VERTEX_SHADER);
  gl.shaderSource(vshader, vertexShaderCode);
  gl.compileShader(vshader);
  if (!gl.getShaderParameter(vshader, gl.COMPILE_STATUS)) 
  {alert('Error during vertex shader compilation:\n' + gl.getShaderInfoLog(vshader)); return;}

  program = gl.createProgram();
  gl.attachShader(program, fshader);
  gl.attachShader(program, vshader);
  gl.linkProgram(program);
  if (!gl.getProgramParameter(program, gl.LINK_STATUS)) 
  {alert('Error during program linking:\n' + gl.getProgramInfoLog(program));return;}

  gl.validateProgram(program);
  if (!gl.getProgramParameter(program, gl.VALIDATE_STATUS)) 
  {alert('Error during program validation:\n' + gl.getProgramInfoLog(program));return;}
  gl.useProgram(program);

  var vattrib = gl.getAttribLocation(program, 'ppos');
  if(vattrib == -1)
  {alert('Error during attribute address retrieval');return;}
  gl.enableVertexAttribArray(vattrib);

  var vbuffer = gl.createBuffer();
  gl.bindBuffer(gl.ARRAY_BUFFER, vbuffer);

  gl.bufferData(gl.ARRAY_BUFFER, vertices, gl.STATIC_DRAW);
  gl.vertexAttribPointer(vattrib, 3, gl.FLOAT, false, 0, 0);

  window.requestAnimationFrame(draw);
}

var sangle = 0.002;
var angle = 0;
var nx = 1;
var ny = 0;
var nz = 0;
var tilt = 1.2;

function draw()
{
  angle += sangle;
  var amvp = gl.getUniformLocation(program, "mvp");
  if(amvp == -1)
  {
  alert('Error during uniform address retrieval');running=false;return;
  }  
  //var as = gl.getUniformLocation(program, "scaler");

  var rot = rotationMatrix(angle, 0, 0, 1);
  var t = rotationMatrix(tilt, nx, ny, nz);
  var sc = scaleMatrix(scale); 
  var mat = multMatrix(multMatrix(rot, t), sc); 
  //var mat = multMatrix(rot, t); 

  gl.uniformMatrix4fv(amvp, false, mat);
  //gl.uniform1f(as, false, scale);

  gl.clearColor(0.0, 0.0, 0.0, 1.0);
  gl.clear(gl.COLOR_BUFFER_BIT);

  gl.drawArrays(method? gl.POINTS : gl.LINE_STRIP, 0, vertices.length/3);
  gl.flush();
  window.requestAnimationFrame(draw);
}

function rotationMatrix(angle, nx, ny, nz)
{
  var l = Math.sqrt(nx * nx + ny * ny + nz * nz);
  nx /= l; ny /= l; nz /= l;
  var c = Math.cos(angle);
  var c1 = 1-c;
  var s = Math.sin(angle);
  return new Float32Array([nx*nx*c1+c,   nx*ny*c1-nz*s,  nx*nz*c1+ny*s, 0,
                           nx*ny*c1+nz*s, ny*ny*c1+c,     ny*nz*c1-nx*s, 0,
                           nx*nz*c1-ny*s, ny*nz*c1+nx*s, nz*nz*c1+c,    0,
                           0,             0,              0,             1]);
}

function scaleMatrix(f)
{
  return new Float32Array([f, 0, 0, 0,
                           0, f, 0, 0,
                           0, 0, f, 0,
                           0, 0, 0, 1]);  
}

function multMatrix(m1, m2)
{
  var v = new Float32Array(16);  
  for(var y = 0; y < 4; y++)
    for(var x = 0; x < 4; x++)
    {
      v[x + y * 4] = 0;
      for(var i = 0; i < 4; i++)
        v[x + y * 4] += m1[i + y * 4] * m2[x + i * 4];
    }
  return v;
}
  </script> 
  <script src="vertices.js"></script>  
  </header>
  <body onload="start();">
  <canvas width="1000" height="1000">
  </canvas><br>
  <button onclick="methodChange()">method</button> rotspeed<input value="0.002" onchange="sangle = this.value * 1" /> tilt<input value="1.2" onchange="tilt = this.value * 1" /> nx<input value="1" onchange="nx = this.value * 1" />  ny<input value="0" onchange="ny = this.value * 1" />  nz<input value="0" onchange="nz = this.value * 1" /> 
  </body>
</html>
)bitluni";
