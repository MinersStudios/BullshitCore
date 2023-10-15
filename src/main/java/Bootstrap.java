public final class Bootstrap {
	public static void main(String[] args) {
		try (java.io.InputStream resource = Bootstrap.class.getClassLoader().getResourceAsStream("Main.js")) {
			final String script = new String(resource.readAllBytes(), java.nio.charset.StandardCharsets.UTF_8);
			try {
				new javax.script.ScriptEngineManager().getEngineByName("js").eval(script);
			} catch (java.lang.NullPointerException e) {
				System.err.println("No ScriptEngine was found!");
				System.exit(1);
			}
		} catch (java.lang.NullPointerException e) {
			System.err.println("Main class is missing!");
			System.exit(1);
		} catch (java.io.IOException | javax.script.ScriptException e) {
			e.printStackTrace();
			System.exit(1);
		}
	}
}
