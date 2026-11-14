import { DEFAULT_FRAGMENT, FULLSCREEN_VERTEX } from "./shaders";

export type CompileResult = { ok: true } | { ok: false; line: number; message: string };

export type GlRenderer = {
  /** 用内置模糊着色器对输入做可分离两遍模糊 */
  blur(source: ImageData, radius: number): ImageData;
  dispose(): void;
};

type Program = {
  program: WebGLProgram;
  uniforms: {
    texelSize: WebGLUniformLocation | null;
    direction: WebGLUniformLocation | null;
    radius: WebGLUniformLocation | null;
    texture: WebGLUniformLocation | null;
  };
};

type Targets = {
  width: number;
  height: number;
  source: WebGLTexture;
  ping: WebGLTexture;
  pong: WebGLTexture;
  framebuffer: WebGLFramebuffer;
};

export class GlUnavailableError extends Error {
  constructor(message: string) {
    super(message);
    this.name = "GlUnavailableError";
  }
}

function lineOf(log: string): number {
  const match = /ERROR:\s*\d+:(\d+)/.exec(log);
  return match ? Number(match[1]) : 1;
}

export function createGlRenderer(fragment = DEFAULT_FRAGMENT): GlRenderer {
  const canvas = new OffscreenCanvas(1, 1);
  const gl = canvas.getContext("webgl2", {
    alpha: true,
    premultipliedAlpha: false,
    preserveDrawingBuffer: false,
  });

  if (!gl) {
    throw new GlUnavailableError("当前系统不支持 WebGL2，无法进行 GPU 特效渲染");
  }

  const compile = (type: number, source: string) => {
    const shader = gl.createShader(type);
    if (!shader) {
      throw new Error("无法创建着色器对象");
    }
    gl.shaderSource(shader, source);
    gl.compileShader(shader);
    if (!gl.getShaderParameter(shader, gl.COMPILE_STATUS)) {
      const log = gl.getShaderInfoLog(shader) ?? "着色器编译失败";
      gl.deleteShader(shader);
      return { shader: null, line: lineOf(log), message: log.trim() } as const;
    }
    return { shader, line: 0, message: "" } as const;
  };

  const buildProgram = (fragmentSource: string): { ok: true; program: Program } | { ok: false; line: number; message: string } => {
    const vertex = compile(gl.VERTEX_SHADER, FULLSCREEN_VERTEX);
    if (!vertex.shader) {
      return { ok: false, line: vertex.line, message: `顶点着色器：${vertex.message}` };
    }
    const fragmentShader = compile(gl.FRAGMENT_SHADER, fragmentSource);
    if (!fragmentShader.shader) {
      gl.deleteShader(vertex.shader);
      return { ok: false, line: fragmentShader.line, message: fragmentShader.message };
    }

    const program = gl.createProgram();
    if (!program) {
      gl.deleteShader(vertex.shader);
      gl.deleteShader(fragmentShader.shader);
      return { ok: false, line: 1, message: "无法创建着色器程序" };
    }

    gl.attachShader(program, vertex.shader);
    gl.attachShader(program, fragmentShader.shader);
    gl.linkProgram(program);
    gl.deleteShader(vertex.shader);
    gl.deleteShader(fragmentShader.shader);

    if (!gl.getProgramParameter(program, gl.LINK_STATUS)) {
      const log = gl.getProgramInfoLog(program) ?? "着色器程序链接失败";
      gl.deleteProgram(program);
      return { ok: false, line: lineOf(log), message: log.trim() };
    }

    return {
      ok: true,
      program: {
        program,
        uniforms: {
          texelSize: gl.getUniformLocation(program, "uTexelSize"),
          direction: gl.getUniformLocation(program, "uDirection"),
          radius: gl.getUniformLocation(program, "uRadius"),
          texture: gl.getUniformLocation(program, "uTexture"),
        },
      },
    };
  };

  const buffer = gl.createBuffer();
  const vao = gl.createVertexArray();
  if (!buffer || !vao) {
    throw new GlUnavailableError("WebGL2 资源创建失败");
  }
  gl.bindVertexArray(vao);
  gl.bindBuffer(gl.ARRAY_BUFFER, buffer);
  gl.bufferData(gl.ARRAY_BUFFER, new Float32Array([-1, -1, 1, -1, -1, 1, 1, 1]), gl.STATIC_DRAW);
  // 顶点着色器固定 a_position 在 location 0，所有程序共用同一个 VAO。
  gl.enableVertexAttribArray(0);
  gl.vertexAttribPointer(0, 2, gl.FLOAT, false, 0, 0);
  gl.bindVertexArray(null);

  const current: Program = (() => {
    const built = buildProgram(fragment);
    if (!built.ok) {
      throw new Error(`内置着色器编译失败：${built.message}`);
    }
    return built.program;
  })();

  let targets: Targets | null = null;

  const ensureTargets = (width: number, height: number): Targets => {
    if (targets && targets.width === width && targets.height === height) {
      return targets;
    }
    if (targets) {
      gl.deleteTexture(targets.source);
      gl.deleteTexture(targets.ping);
      gl.deleteTexture(targets.pong);
      gl.deleteFramebuffer(targets.framebuffer);
    }

    const createTexture = () => {
      const texture = gl.createTexture();
      if (!texture) {
        throw new GlUnavailableError("无法创建纹理");
      }
      gl.bindTexture(gl.TEXTURE_2D, texture);
      gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR);
      gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR);
      gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
      gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
      gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, width, height, 0, gl.RGBA, gl.UNSIGNED_BYTE, null);
      return texture;
    };

    const framebuffer = gl.createFramebuffer();
    if (!framebuffer) {
      throw new GlUnavailableError("无法创建帧缓冲");
    }

    targets = {
      width,
      height,
      source: createTexture(),
      ping: createTexture(),
      pong: createTexture(),
      framebuffer,
    };
    return targets;
  };

  const drawPass = (
    input: WebGLTexture,
    output: WebGLTexture,
    size: { width: number; height: number },
    direction: [number, number],
    radius: number,
    framebuffer: WebGLFramebuffer,
  ) => {
    gl.bindFramebuffer(gl.FRAMEBUFFER, framebuffer);
    gl.framebufferTexture2D(gl.FRAMEBUFFER, gl.COLOR_ATTACHMENT0, gl.TEXTURE_2D, output, 0);
    gl.viewport(0, 0, size.width, size.height);

    gl.useProgram(current.program);
    gl.bindTexture(gl.TEXTURE_2D, input);
    gl.uniform1i(current.uniforms.texture, 0);
    gl.uniform2f(current.uniforms.texelSize, 1 / size.width, 1 / size.height);
    gl.uniform2f(current.uniforms.direction, direction[0], direction[1]);
    gl.uniform1f(current.uniforms.radius, radius);

    gl.bindVertexArray(vao);
    gl.drawArrays(gl.TRIANGLE_STRIP, 0, 4);
    gl.bindVertexArray(null);
  };

  return {
    blur(source: ImageData, radius: number): ImageData {
      const size = { width: source.width, height: source.height };
      const target = ensureTargets(size.width, size.height);

      gl.bindTexture(gl.TEXTURE_2D, target.source);
      gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, size.width, size.height, 0, gl.RGBA, gl.UNSIGNED_BYTE, source.data);

      drawPass(target.source, target.ping, size, [1, 0], radius, target.framebuffer);
      drawPass(target.ping, target.pong, size, [0, 1], radius, target.framebuffer);

      gl.bindFramebuffer(gl.FRAMEBUFFER, target.framebuffer);
      gl.framebufferTexture2D(gl.FRAMEBUFFER, gl.COLOR_ATTACHMENT0, gl.TEXTURE_2D, target.pong, 0);

      const pixels = new Uint8ClampedArray(new ArrayBuffer(size.width * size.height * 4));
      gl.readPixels(0, 0, size.width, size.height, gl.RGBA, gl.UNSIGNED_BYTE, pixels);
      gl.bindFramebuffer(gl.FRAMEBUFFER, null);

      return new ImageData(pixels, size.width, size.height);
    },

    dispose() {
      if (targets) {
        gl.deleteTexture(targets.source);
        gl.deleteTexture(targets.ping);
        gl.deleteTexture(targets.pong);
        gl.deleteFramebuffer(targets.framebuffer);
        targets = null;
      }
      gl.deleteProgram(current.program);
      gl.deleteVertexArray(vao);
      gl.deleteBuffer(buffer);
    },
  };
}
