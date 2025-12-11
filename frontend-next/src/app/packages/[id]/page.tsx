import type { Metadata } from "next";
import Link from "next/link";
import { notFound } from "next/navigation";
import { getPackages, getPackageManifest, type Manifest } from "@/lib/registry";

interface PageProps {
  params: Promise<{ id: string }>;
}

export async function generateStaticParams() {
  const packages = await getPackages();
  return packages.map((pkg) => ({ id: pkg.id }));
}

export async function generateMetadata({ params }: PageProps): Promise<Metadata> {
  const { id } = await params;
  const packages = await getPackages();
  const pkg = packages.find((p) => p.id === id);
  return {
    title: pkg?.name || id,
    description: pkg?.description || `Details for ${id}`,
  };
}

export default async function PackageDetailPage({ params }: PageProps) {
  const { id } = await params;
  const packages = await getPackages();
  const pkg = packages.find((p) => p.id === id);

  if (!pkg) {
    notFound();
  }

  let manifest: Manifest | null = null;
  if (pkg.manifest) {
    manifest = await getPackageManifest(pkg.manifest);
  }

  const data = manifest || pkg;

  return (
    <>
      <div className="bg-white border-b border-gray-200 pt-8">
        <div className="container mx-auto px-4">
          <div className="flex items-baseline gap-4 mb-2">
            <h1 className="text-2xl font-semibold text-gray-900">{data.name}</h1>
            <span className="text-gray-500 text-base font-mono">v{data.version}</span>
            {manifest?.license && (
              <span className="text-xs border border-gray-200 rounded-full px-2.5 py-0.5 text-gray-500">
                {manifest.license}
              </span>
            )}
          </div>
          <p className="text-lg text-gray-600 mb-6 max-w-3xl">{data.description}</p>
          
          <div className="flex gap-8 border-b border-transparent">
            <Link href="#" className="text-gray-900 py-3 text-sm font-medium border-b-2 border-[#cb3837]">
              Readme
            </Link>
            <Link href="#" className="text-gray-500 py-3 text-sm font-medium border-b-2 border-transparent hover:text-gray-900 hover:border-gray-300">
              Code
            </Link>
            <Link href="#" className="text-gray-500 py-3 text-sm font-medium border-b-2 border-transparent hover:text-gray-900 hover:border-gray-300">
              Dependencies
            </Link>
            <Link href="#" className="text-gray-500 py-3 text-sm font-medium border-b-2 border-transparent hover:text-gray-900 hover:border-gray-300">
              Versions
            </Link>
          </div>
        </div>
      </div>

      <main className="container mx-auto px-4 py-8">
        <div className="grid grid-cols-1 lg:grid-cols-[1fr_350px] gap-12">
          <div className="space-y-10">
            <section>
              <h2 className="text-2xl font-semibold mb-4 pb-2 border-b border-gray-200">Usage</h2>
              <div className="bg-[#f6f8fa] border border-gray-200 rounded-md overflow-hidden">
                <div className="flex justify-between items-center px-4 py-2 border-b border-gray-200 bg-[#f1f3f5]">
                  <span className="text-sm text-gray-500 font-medium">Run this package</span>
                  <button className="text-gray-500 hover:text-gray-900">
                    <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" className="w-4 h-4">
                      <rect x="9" y="9" width="13" height="13" rx="2" />
                      <path d="M5 15H4a2 2 0 01-2-2V4a2 2 0 012-2h9a2 2 0 012 2v1" />
                    </svg>
                  </button>
                </div>
                <div className="p-4 overflow-x-auto">
                  <code className="font-mono text-sm text-gray-900">nex run {data.id}</code>
                </div>
              </div>
            </section>

            {manifest?.commands && Object.keys(manifest.commands).length > 0 && (
              <section>
                <h2 className="text-2xl font-semibold mb-4 pb-2 border-b border-gray-200">Available Commands</h2>
                <div className="border border-gray-200 rounded-md overflow-hidden">
                  <table className="w-full border-collapse">
                    <thead>
                      <tr className="bg-[#f6f8fa]">
                        <th className="text-left px-4 py-3 font-semibold text-sm border-b border-gray-200">Command</th>
                        <th className="text-left px-4 py-3 font-semibold text-sm border-b border-gray-200">Runs</th>
                      </tr>
                    </thead>
                    <tbody>
                      {Object.entries(manifest.commands).map(([name, cmd]) => (
                        <tr key={name} className="border-b border-gray-200 last:border-0">
                          <td className="px-4 py-3">
                            <code className="bg-[#f6f8fa] px-2 py-1 rounded text-sm font-mono">nex run {data.id} {name}</code>
                          </td>
                          <td className="px-4 py-3">
                            <code className="text-gray-600 font-mono text-sm">{cmd}</code>
                          </td>
                        </tr>
                      ))}
                    </tbody>
                  </table>
                </div>
              </section>
            )}

            {data.keywords && data.keywords.length > 0 && (
              <section>
                <h2 className="text-2xl font-semibold mb-4 pb-2 border-b border-gray-200">Keywords</h2>
                <div className="flex flex-wrap gap-2">
                  {data.keywords.map((kw) => (
                    <Link href="#" key={kw} className="text-[#cb3837] bg-[#fff0f0] px-3 py-1 rounded text-sm font-medium hover:bg-[#ffe0e0] transition-colors">
                      {kw}
                    </Link>
                  ))}
                </div>
              </section>
            )}
          </div>

          <aside className="space-y-8">
            <div>
              <h3 className="text-xs font-semibold text-gray-500 uppercase tracking-wide mb-3">Install</h3>
              <div className="flex items-center gap-2 bg-white border border-gray-200 p-3 rounded-md text-gray-900">
                <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" className="w-4 h-4 text-gray-500">
                  <path d="M9 19c-5 1.5-5-2.5-7-3m14 6v-3.87a3.37 3.37 0 0 0-.94-2.61c3.14-.35 6.44-1.54 6.44-7A5.44 5.44 0 0 0 20 4.77 5.07 5.07 0 0 0 19.91 1S18.73.65 16 2.48a13.38 13.38 0 0 0-7 0C6.27.65 5.09 1 5.09 1A5.07 5.07 0 0 0 5 4.77a5.44 5.44 0 0 0-1.5 3.78c0 5.42 3.3 6.61 6.44 7A3.37 3.37 0 0 0 9 18.13V22" />
                </svg>
                <code className="flex-1 font-mono text-sm truncate">nex install {data.id}</code>
                <button className="text-gray-500 hover:text-gray-900 p-1 rounded hover:bg-gray-100">
                  <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" className="w-4 h-4">
                    <rect x="9" y="9" width="13" height="13" rx="2" />
                    <path d="M5 15H4a2 2 0 01-2-2V4a2 2 0 012-2h9a2 2 0 012 2v1" />
                  </svg>
                </button>
              </div>
            </div>

            <div>
              <h3 className="text-xs font-semibold text-gray-500 uppercase tracking-wide mb-3">Repository</h3>
              {manifest?.repository ? (
                <a href={manifest.repository} target="_blank" rel="noopener noreferrer" className="flex items-center gap-2 text-gray-900 font-medium hover:text-[#cb3837] hover:underline">
                  <svg viewBox="0 0 24 24" fill="currentColor" className="w-4 h-4">
                    <path d="M12 0C5.37 0 0 5.37 0 12c0 5.31 3.435 9.795 8.205 11.385.6.105.825-.255.825-.57 0-.285-.015-1.23-.015-2.235-3.015.555-3.795-.735-4.035-1.41-.135-.345-.72-1.41-1.23-1.695-.42-.225-1.02-.78-.015-.795.945-.015 1.62.87 1.845 1.23 1.08 1.815 2.805 1.305 3.495.99.105-.78.42-1.305.765-1.605-2.67-.3-5.46-1.335-5.46-5.925 0-1.305.465-2.385 1.23-3.225-.12-.3-.54-1.53.12-3.18 0 0 1.005-.315 3.3 1.23.96-.27 1.98-.405 3-.405s2.04.135 3 .405c2.295-1.56 3.3-1.23 3.3-1.23.66 1.65.24 2.88.12 3.18.765.84 1.23 1.905 1.23 3.225 0 4.605-2.805 5.625-5.475 5.925.435.375.81 1.095.81 2.22 0 1.605-.015 2.895-.015 3.3 0 .315.225.69.825.57A12.02 12.02 0 0024 12c0-6.63-5.37-12-12-12z" />
                  </svg>
                  <span>{manifest.repository.replace('https://github.com/', '')}</span>
                </a>
              ) : (
                <span className="text-gray-500 text-sm">No repository linked</span>
              )}
            </div>

            <div>
              <h3 className="text-xs font-semibold text-gray-500 uppercase tracking-wide mb-3">Details</h3>
              <div className="space-y-2">
                {manifest?.author && (
                  <div className="flex justify-between items-baseline text-sm border-b border-gray-100 pb-2">
                    <span className="text-gray-500 font-medium">Author</span>
                    <span className="text-gray-900 font-semibold">
                      {typeof manifest.author === "string" ? manifest.author : (
                        manifest.author.github ? (
                          <a href={`https://github.com/${manifest.author.github}`} target="_blank" rel="noopener noreferrer" className="hover:underline">
                            {manifest.author.name || manifest.author.github}
                          </a>
                        ) : manifest.author.name
                      )}
                    </span>
                  </div>
                )}
                
                <div className="flex justify-between items-baseline text-sm border-b border-gray-100 pb-2">
                  <span className="text-gray-500 font-medium">Version</span>
                  <span className="text-gray-900 font-semibold">{data.version}</span>
                </div>

                {manifest?.license && (
                  <div className="flex justify-between items-baseline text-sm border-b border-gray-100 pb-2">
                    <span className="text-gray-500 font-medium">License</span>
                    <span className="text-gray-900 font-semibold">{manifest.license}</span>
                  </div>
                )}

                {manifest?.runtime && (
                  <div className="flex justify-between items-baseline text-sm border-b border-gray-100 pb-2">
                    <span className="text-gray-500 font-medium">Runtime</span>
                    <span className="text-gray-900 font-semibold">{manifest.runtime.type} {manifest.runtime.version}</span>
                  </div>
                )}
              </div>
            </div>

            {manifest?.platforms && (
              <div>
                <h3 className="text-xs font-semibold text-gray-500 uppercase tracking-wide mb-3">Platforms</h3>
                <div className="flex flex-wrap gap-2">
                  {manifest.platforms.map((p) => (
                    <span key={p} className="text-xs px-2 py-0.5 border border-gray-200 rounded-full text-gray-500 bg-white">
                      {p}
                    </span>
                  ))}
                </div>
              </div>
            )}
          </aside>
        </div>
      </main>
    </>
  );
}
